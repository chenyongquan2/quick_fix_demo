#include "fix_app_orchestrator.h"

#include <spdlog/spdlog.h>

FixAppOrchestrator::FixAppOrchestrator(std::unique_ptr<DomainService> svc, std::unique_ptr<FixSender> fix_sender)
    : svc_(std::move(svc))
    , fix_sender_(std::move(fix_sender))
{
    svc_->setOrderStatusCallback([this](const common::Order& o, const std::string& st) {
        auto sid = getSessionId();
        if (!sid)
            return;
        sendExecutionReport(o, "EXEC_STATUS", "8", st, *sid); // ExecType '8' just as placeholder for status update
    });
    svc_->setMarginUpdateCallback([this](const common::MarginUpdate& mu) { sendMargin(mu); });
}

void FixAppOrchestrator::onCreate(const FIX::SessionID& sessionID)
{
    SPDLOG_INFO("onCreate: {}", sessionID.toString());
}

void FixAppOrchestrator::onLogon(const FIX::SessionID& sessionID)
{
    SPDLOG_INFO("onLogon: {}", sessionID.toString());
    setSessionId(sessionID);
    svc_->startMarginUpdates();
}

void FixAppOrchestrator::onLogout(const FIX::SessionID& sessionID)
{
    SPDLOG_INFO("onLogout: {}", sessionID.toString());
    svc_->stopMarginUpdates();
    clearSessionId();
}

void FixAppOrchestrator::toAdmin(FIX::Message& message, const FIX::SessionID& sessionID)
{
    SPDLOG_DEBUG("toAdmin: {}", message.toString());
}

void FixAppOrchestrator::fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID)
{
    SPDLOG_DEBUG("fromAdmin: {}", message.toString());
}

void FixAppOrchestrator::toApp(FIX::Message& message, const FIX::SessionID& sessionID)
{
    SPDLOG_INFO("toApp:   {}", message.toString());
}

void FixAppOrchestrator::onFromApp(const FIX::Message& message, const FIX::SessionID& sessionID)
{
    try {
        SPDLOG_INFO("fromApp: {}", message.toString());
        crack(message, sessionID);
    } catch (const std::exception& ex) {
        SPDLOG_ERROR("fromApp error: {}", ex.what());
    } catch (...) {
        SPDLOG_ERROR("fromApp unknown error");
    }
}

void FixAppOrchestrator::onMessage(const FIX44::NewOrderSingle& nos, const FIX::SessionID& sessionID)
{
    try {
        auto order = FixMessageConverter::parseNewOrderSingle(nos);
        SPDLOG_INFO("Processing NewOrderSingle: ClOrdID={}, Symbol={}, Qty={}, Price={}", order.clOrdId, order.symbol,
            order.quantity, order.price);

        auto r = svc_->processNewOrder(order);
        if (r.success) {
            SPDLOG_INFO("Order accepted: ClOrdID={}, OrderID={}", order.clOrdId, r.orderId);
            sendExecutionReport(r.updatedOrder, r.execId, "0", "0", sessionID); // NEW/NEW
        } else {
            SPDLOG_WARN("Order rejected: ClOrdID={}, Reason={}", order.clOrdId, r.message);
            sendOrderReject(order.clOrdId, r.message, sessionID);
        }
    } catch (const std::exception& ex) {
        SPDLOG_ERROR("NOS error: {}", ex.what());
        // 发送拒绝消息
        try {
            FIX::ClOrdID clOrdId;
            nos.get(clOrdId);
            sendOrderReject(clOrdId.getValue(), "Internal error processing order", sessionID);
        } catch (...) {
            SPDLOG_ERROR("Failed to send rejection message");
        }
    }
}

void FixAppOrchestrator::onMessage(const FIX44::OrderCancelRequest& ocr, const FIX::SessionID& sessionID)
{
    try {
        std::string orig;
        auto order = FixMessageConverter::parseCancelRequest(ocr, orig);
        SPDLOG_INFO("Processing OrderCancelRequest: ClOrdID={}, OrigClOrdID={}", order.clOrdId, orig);

        auto r = svc_->processCancelOrder(order, orig);
        if (r.success) {
            SPDLOG_INFO("Cancel order accepted: ClOrdID={}, OrderID={}", order.clOrdId, r.orderId);
            sendExecutionReport(r.updatedOrder, r.execId, "4", "4", sessionID); // CANCELED
        } else {
            SPDLOG_WARN("Cancel order rejected: ClOrdID={}, Reason={}", order.clOrdId, r.message);
            sendOrderReject(order.clOrdId, r.message, sessionID);
        }
    } catch (const std::exception& ex) {
        SPDLOG_ERROR("CxlReq error: {}", ex.what());
        // 发送拒绝消息
        try {
            FIX::ClOrdID clOrdId;
            ocr.get(clOrdId);
            sendOrderReject(clOrdId.getValue(), "Internal error processing cancel request", sessionID);
        } catch (...) {
            SPDLOG_ERROR("Failed to send cancel rejection message");
        }
    }
}

void FixAppOrchestrator::onMessage(const FIX44::OrderCancelReplaceRequest& ocrr, const FIX::SessionID& sessionID)
{
    try {
        std::string orig;
        auto order = FixMessageConverter::parseReplaceRequest(ocrr, orig);
        SPDLOG_INFO("Processing OrderCancelReplaceRequest: ClOrdID={}, OrigClOrdID={}, Qty={}, Price={}", order.clOrdId,
            orig, order.quantity, order.price);

        auto r = svc_->processReplaceOrder(order, orig);
        if (r.success) {
            SPDLOG_INFO("Replace order accepted: ClOrdID={}, OrderID={}", order.clOrdId, r.orderId);
            sendExecutionReport(r.updatedOrder, r.execId, "5", "5", sessionID); // REPLACED
        } else {
            SPDLOG_WARN("Replace order rejected: ClOrdID={}, Reason={}", order.clOrdId, r.message);
            sendOrderReject(order.clOrdId, r.message, sessionID);
        }
    } catch (const std::exception& ex) {
        SPDLOG_ERROR("OCrr error: {}", ex.what());
        // 发送拒绝消息
        try {
            FIX::ClOrdID clOrdId;
            ocrr.get(clOrdId);
            sendOrderReject(clOrdId.getValue(), "Internal error processing replace request", sessionID);
        } catch (...) {
            SPDLOG_ERROR("Failed to send replace rejection message");
        }
    }
}

void FixAppOrchestrator::sendExecutionReport(const common::Order& order, const std::string& execId,
    const std::string& execType, const std::string& ordStatus, const FIX::SessionID& sessionID)
{
    auto m = FixMessageConverter::createExecutionReport(order, execId, execType, ordStatus, sessionID);
    fix_sender_->sendToTarget(m, sessionID);
}

void FixAppOrchestrator::sendOrderReject(
    const std::string& clOrdId, const std::string& reason, const FIX::SessionID& sessionID)
{
    auto m = FixMessageConverter::createOrderReject(clOrdId, reason, sessionID);
    fix_sender_->sendToTarget(m, sessionID);
}

void FixAppOrchestrator::sendMargin(const common::MarginUpdate& mu)
{
    auto sid = getSessionId();
    if (!sid)
        return;
    auto m = FixMessageConverter::createMarginUpdate(mu, *sid);
    fix_sender_->sendToTarget(m, *sid);
}

void FixAppOrchestrator::setSessionId(const FIX::SessionID& sessionID)
{
    std::lock_guard<std::mutex> lk(session_mtx_);
    session_id_ = sessionID;
}

void FixAppOrchestrator::clearSessionId()
{
    std::lock_guard<std::mutex> lk(session_mtx_);
    session_id_.reset();
}

std::optional<FIX::SessionID> FixAppOrchestrator::getSessionId()
{
    std::lock_guard<std::mutex> lk(session_mtx_);
    return session_id_;
}
