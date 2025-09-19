#pragma once

#include <quickfix/Application.h>
#include <quickfix/Session.h>
#include <quickfix/Fields.h>

#include <memory>

#include "fix_app_orchestrator.h"

class InitiatorApplication : public FIX::Application {
public:
    InitiatorApplication();
    ~InitiatorApplication() = default;
    void onCreate(const FIX::SessionID& sessionID) override;
    void onLogon(const FIX::SessionID& sessionID) override;
    void onLogout(const FIX::SessionID& sessionID) override;
    void toAdmin(FIX::Message& message, const FIX::SessionID& sessionID) override;
    void fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID) override;
    void toApp(FIX::Message& message, const FIX::SessionID& sessionID) override;
    void fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) override;

private:
    std::unique_ptr<FixAppOrchestrator> orchestrator_;
};
