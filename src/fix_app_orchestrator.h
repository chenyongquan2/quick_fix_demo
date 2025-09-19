#pragma once

#include <quickfix/MessageCracker.h>
#include <quickfix/Session.h>
#include <quickfix/fix44/NewOrderSingle.h>
#include <quickfix/fix44/OrderCancelRequest.h>
#include <quickfix/fix44/OrderCancelReplaceRequest.h>

#include "domain_service.h"
#include "fix_sender.h"
#include "fix_message_converter.h"

#include <memory>
#include <mutex>
#include <optional>

class FixAppOrchestrator : public FIX::MessageCracker {
public:
    explicit FixAppOrchestrator(
        std::unique_ptr<DomainService> svc, std::unique_ptr<FixSender> fix_sender = std::make_unique<QuickFixSender>());

    void onCreate(const FIX::SessionID& sessionID);
    void onLogon(const FIX::SessionID& sessionID);
    void onLogout(const FIX::SessionID& sessionID);
    void toAdmin(FIX::Message& message, const FIX::SessionID& sessionID);
    void fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID);
    void toApp(FIX::Message& message, const FIX::SessionID& sessionID);

    void onFromApp(const FIX::Message& message, const FIX::SessionID& sessionID);

    void onMessage(const FIX44::NewOrderSingle& nos, const FIX::SessionID& sessionID) override;
    void onMessage(const FIX44::OrderCancelRequest& ocr, const FIX::SessionID& sessionID) override;
    void onMessage(const FIX44::OrderCancelReplaceRequest& ocrr, const FIX::SessionID& sessionID) override;

private:
    void sendExecutionReport(const common::Order& order, const std::string& execId, const std::string& execType,
        const std::string& ordStatus, const FIX::SessionID& sessionID);
    void sendOrderReject(const std::string& clOrdId, const std::string& reason, const FIX::SessionID& sessionID);
    void sendMargin(const common::MarginUpdate& mu);

    void setSessionId(const FIX::SessionID& sessionID);
    void clearSessionId();
    std::optional<FIX::SessionID> getSessionId();

private:
    std::unique_ptr<DomainService> svc_;
    std::unique_ptr<FixSender> fix_sender_;
    std::mutex session_mtx_;
    std::optional<FIX::SessionID> session_id_;
};
