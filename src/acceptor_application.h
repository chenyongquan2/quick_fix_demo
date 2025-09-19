#pragma once

#include <quickfix/Application.h>
#include <quickfix/MessageCracker.h>
#include <quickfix/Session.h>
#include <quickfix/fix44/ExecutionReport.h>
#include <quickfix/fix44/OrderCancelReject.h>

#include <string>

class AcceptorApplication : public FIX::Application, public FIX::MessageCracker {
public:
    void onCreate(const FIX::SessionID& sessionID) override;
    void onLogon(const FIX::SessionID& sessionID) override;
    void onLogout(const FIX::SessionID& sessionID) override;
    void toAdmin(FIX::Message& message, const FIX::SessionID& sessionID) override;
    void fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID) override;
    void toApp(FIX::Message& message, const FIX::SessionID& sessionID) override;
    void fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) override;

    void onMessage(const FIX44::ExecutionReport& er, const FIX::SessionID& sessionID) override;
    void onMessage(const FIX44::OrderCancelReject& rej, const FIX::SessionID& sessionID) override;

    void handleMarginUpdateMessage(const FIX::Message& msg, const FIX::SessionID& sessionID);

private:
    //mock sending new order request from BlackArrow to Doo fix engine
    void startTestTask();
    void testNewMarketOrder();
    void testNewLimitOrder();
    void testCancelOrder();
    void testReplaceOrder();
    void testNewnvalidOrder();
    static std::string generateClOrdId();

    std::string lastClOrdId_;

private:
    FIX::SessionID session_;

};


