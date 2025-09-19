#include "acceptor_application.h"

#include <quickfix/SessionSettings.h>
#include <quickfix/FileStore.h>
#include <quickfix/FileLog.h>
#include <quickfix/SocketInitiator.h>
#include <quickfix/fix44/NewOrderSingle.h>
#include <quickfix/fix44/OrderCancelRequest.h>
#include <quickfix/fix44/OrderCancelReplaceRequest.h>
#include <quickfix/Fields.h>

#include <spdlog/spdlog.h>

#include <thread>
#include <chrono>
#include <random>
#include <iostream>

#include "fix_custom.h"

void AcceptorApplication::onCreate(const FIX::SessionID& sessionID)
{
    SPDLOG_INFO("onCreate: {}", sessionID.toString());
}

void AcceptorApplication::onLogon(const FIX::SessionID& sessionID)
{
    SPDLOG_INFO("onLogon:  {}", sessionID.toString());
    session_ = sessionID;
    startTestTask();
}

void AcceptorApplication::onLogout(const FIX::SessionID& sessionID)
{
    SPDLOG_INFO("onLogout: {}", sessionID.toString());
}

void AcceptorApplication::toAdmin(FIX::Message& message, const FIX::SessionID& sessionID)
{
    SPDLOG_DEBUG("toAdmin:  {}", message.toString());
}

void AcceptorApplication::fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID)
{
    SPDLOG_DEBUG("fromAdmin:{}", message.toString());
}

void AcceptorApplication::toApp(FIX::Message& message, const FIX::SessionID& sessionID)
{
    SPDLOG_INFO("toApp:    {}", message.toString());
}

void AcceptorApplication::fromApp(const FIX::Message& message, const FIX::SessionID& sessionID)
{
    SPDLOG_INFO("fromApp:  {}", message.toString());
    try {
        FIX::MsgType mt;
        message.getHeader().getField(mt);
        if (mt.getValue() == fixcustom::kMsgTypeMarginUpdate) {
            handleMarginUpdateMessage(message, sessionID);
            return;
        }
    } catch (const std::exception& ex) {
        SPDLOG_ERROR("fromApp BI check error: {}", ex.what());
    }
    crack(message, sessionID);
}

void AcceptorApplication::onMessage(const FIX44::ExecutionReport& er, const FIX::SessionID& sessionID)
{
    try {
        FIX::ExecType execType;
        er.get(execType);
        FIX::OrdStatus ordStatus;
        er.get(ordStatus);
        FIX::ClOrdID clOrdId;
        if (er.isSetField(clOrdId))
            er.get(clOrdId);
        else
            clOrdId = FIX::ClOrdID("");
        SPDLOG_INFO(
            "ER: ClOrdID={}, ExecType={}, OrdStatus={}", clOrdId.getValue(), execType.getValue(), ordStatus.getValue());
    } catch (const std::exception& ex) {
        SPDLOG_ERROR("onMessage(ER) error: {}", ex.what());
    }
}

void AcceptorApplication::onMessage(const FIX44::OrderCancelReject& rej, const FIX::SessionID& sessionID)
{
    try {
        FIX::ClOrdID clOrdId;
        if (rej.isSetField(clOrdId))
            rej.get(clOrdId);
        else
            clOrdId = FIX::ClOrdID("");
        FIX::Text text;
        if (rej.isSetField(text))
            rej.get(text);
        else
            text = FIX::Text("");
        SPDLOG_INFO("CxlReject: ClOrdID={}, Text={}", clOrdId.getValue(), text.getValue());
    } catch (const std::exception& ex) {
        SPDLOG_ERROR("onMessage(9) error: {}", ex.what());
    }
}

void AcceptorApplication::handleMarginUpdateMessage(const FIX::Message& msg, const FIX::SessionID& sessionID)
{
    try {
        std::string account;
        double marginValue = 0.0;
        double marginLevel = 0.0;
        double marginExcess = 0.0;
        std::string currency;

        if (msg.isSetField(fixcustom::TAG_ACCOUNT)) {
            FIX::Account acc;
            msg.getField(acc);
            account = acc.getValue();
        }
        if (msg.isSetField(fixcustom::TAG_MARGIN_VALUE)) {
            FIX::FieldBase f(fixcustom::TAG_MARGIN_VALUE, "");
            msg.getField(f);
            try {
                marginValue = std::stod(f.getString());
            } catch (const std::exception& ex) {
                SPDLOG_ERROR("Failed to parse margin value: {}", f.getString());
                return;
            }
        }
        if (msg.isSetField(fixcustom::TAG_MARGIN_LEVEL)) {
            FIX::FieldBase f(fixcustom::TAG_MARGIN_LEVEL, "");
            msg.getField(f);
            try {
                marginLevel = std::stod(f.getString());
            } catch (const std::exception& ex) {
                SPDLOG_ERROR("Failed to parse margin level: {}", f.getString());
                return;
            }
        }
        if (msg.isSetField(fixcustom::TAG_MARGIN_EXCESS)) {
            FIX::FieldBase f(fixcustom::TAG_MARGIN_EXCESS, "");
            msg.getField(f);
            try {
                marginExcess = std::stod(f.getString());
            } catch (const std::exception& ex) {
                SPDLOG_ERROR("Failed to parse margin excess: {}", f.getString());
                return;
            }
        }
        if (msg.isSetField(fixcustom::TAG_CURRENCY)) {
            FIX::Currency curr;
            msg.getField(curr);
            currency = curr.getValue();
        }

        SPDLOG_INFO("Received margin update: account={}, marginValue={:.2f}, marginLevel={:.2f}%, marginExcess={:.2f}, "
                    "currency={}",
            account, marginValue, marginLevel, marginExcess, currency);
    } catch (const std::exception& ex) {
        SPDLOG_ERROR("handleMarginUpdateMessage error: {}", ex.what());
    }
}

void AcceptorApplication::startTestTask()
{
    std::thread([this]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        testNewMarketOrder();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        testNewLimitOrder();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        testCancelOrder();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        testReplaceOrder();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        testNewnvalidOrder();
    }).detach();
}

void AcceptorApplication::testNewMarketOrder()
{
    try {
        FIX::Session* session = FIX::Session::lookupSession(session_);
        if (!session)
            return;

        std::string clOrdId = generateClOrdId();
        FIX44::NewOrderSingle nos;
        nos.setField(FIX::ClOrdID(clOrdId));
        nos.setField(FIX::Side(FIX::Side_BUY));
        nos.setField(FIX::TransactTime());
        nos.setField(FIX::OrdType(FIX::OrdType_MARKET));
        nos.setField(FIX::Symbol("AAPL"));
        nos.setField(FIX::OrderQty(100));
        nos.setField(FIX::Account("TEST-001"));
        nos.setField(FIX::TimeInForce(FIX::TimeInForce_DAY));

        FIX::Session::sendToTarget(nos, session_);
        SPDLOG_INFO("Sent market order: ClOrdID={}, Symbol=AAPL, Qty=100", clOrdId);
        lastClOrdId_ = clOrdId;
    } catch (const std::exception& ex) {
        SPDLOG_ERROR("testNewMarketOrder error: {}", ex.what());
    }
}

void AcceptorApplication::testNewLimitOrder()
{
    try {
        FIX::Session* session = FIX::Session::lookupSession(session_);
        if (!session)
            return;

        std::string clOrdId = generateClOrdId();
        FIX44::NewOrderSingle nos;
        nos.setField(FIX::ClOrdID(clOrdId));
        nos.setField(FIX::Side(FIX::Side_SELL));
        nos.setField(FIX::TransactTime());
        nos.setField(FIX::OrdType(FIX::OrdType_LIMIT));
        nos.setField(FIX::Symbol("MSFT"));
        nos.setField(FIX::OrderQty(50));
        nos.setField(FIX::Price(150.25));
        nos.setField(FIX::Account("TEST-001"));
        nos.setField(FIX::TimeInForce(FIX::TimeInForce_DAY));

        FIX::Session::sendToTarget(nos, session_);
        SPDLOG_INFO("Sent limit order: ClOrdID={}, Symbol=MSFT, Qty=50, Price=150.25", clOrdId);
        lastClOrdId_ = clOrdId;
    } catch (const std::exception& ex) {
        SPDLOG_ERROR("testNewLimitOrder error: {}", ex.what());
    }
}

void AcceptorApplication::testCancelOrder()
{
    try {
        FIX::Session* session = FIX::Session::lookupSession(session_);
        if (!session)
            return;

        std::string clOrdId = generateClOrdId();
        FIX44::OrderCancelRequest ocr;
        ocr.setField(FIX::ClOrdID(clOrdId));
        ocr.setField(FIX::OrigClOrdID(lastClOrdId_));
        ocr.setField(FIX::Symbol("AAPL"));
        ocr.setField(FIX::Side(FIX::Side_BUY));
        ocr.setField(FIX::TransactTime());

        FIX::Session::sendToTarget(ocr, session_);
        SPDLOG_INFO("Sent cancel request: ClOrdID={}, OrigClOrdID={}", clOrdId, lastClOrdId_);
    } catch (const std::exception& ex) {
        SPDLOG_ERROR("testCancelOrder error: {}", ex.what());
    }
}

void AcceptorApplication::testReplaceOrder()
{
    try {
        FIX::Session* session = FIX::Session::lookupSession(session_);
        if (!session)
            return;

        std::string clOrdId = generateClOrdId();
        FIX44::OrderCancelReplaceRequest ocrr;
        ocrr.setField(FIX::ClOrdID(clOrdId));
        ocrr.setField(FIX::OrigClOrdID(lastClOrdId_));
        ocrr.setField(FIX::Symbol("AAPL"));
        ocrr.setField(FIX::Side(FIX::Side_BUY));
        ocrr.setField(FIX::OrderQty(200));
        ocrr.setField(FIX::Price(155.50));
        ocrr.setField(FIX::OrdType(FIX::OrdType_LIMIT));
        ocrr.setField(FIX::TransactTime());

        FIX::Session::sendToTarget(ocrr, session_);
        SPDLOG_INFO("Sent replace request: ClOrdID={}, OrigClOrdID={}, Qty=200, Price=155.50", clOrdId, lastClOrdId_);
    } catch (const std::exception& ex) {
        SPDLOG_ERROR("testReplaceOrder error: {}", ex.what());
    }
}

void AcceptorApplication::testNewnvalidOrder()
{
    try {
        FIX::Session* session = FIX::Session::lookupSession(session_);
        if (!session)
            return;

        std::string clOrdId = generateClOrdId();
        FIX44::NewOrderSingle nos;
        nos.setField(FIX::ClOrdID(clOrdId));
        nos.setField(FIX::Side(FIX::Side_BUY));
        nos.setField(FIX::TransactTime());
        nos.setField(FIX::OrdType(FIX::OrdType_MARKET));
        nos.setField(FIX::Symbol("INVALID"));
        nos.setField(FIX::OrderQty(-10));
        nos.setField(FIX::Account("TEST-001"));

        FIX::Session::sendToTarget(nos, session_);
        SPDLOG_INFO("Sent invalid order: ClOrdID={}, Symbol=INVALID, Qty=-10", clOrdId);
    } catch (const std::exception& ex) {
        SPDLOG_ERROR("testNewnvalidOrder error: {}", ex.what());
    }
}

std::string AcceptorApplication::generateClOrdId()
{
    static std::mt19937_64 rng { std::random_device {}() };
    return std::to_string(rng());
}
