#include "initiator_application.h"

#include <spdlog/spdlog.h>

InitiatorApplication::InitiatorApplication()
    // inject domain service instead of constructing it in the constructor to make it more convenient for unit-testing
    : orchestrator_(std::make_unique<FixAppOrchestrator>(std::make_unique<DomainService>()))
{
}

void InitiatorApplication::onCreate(const FIX::SessionID& sessionID) { orchestrator_->onCreate(sessionID); }

void InitiatorApplication::onLogon(const FIX::SessionID& sessionID) { orchestrator_->onLogon(sessionID); }

void InitiatorApplication::onLogout(const FIX::SessionID& sessionID) { orchestrator_->onLogout(sessionID); }

void InitiatorApplication::toAdmin(FIX::Message& message, const FIX::SessionID& sessionID)
{
    orchestrator_->toAdmin(message, sessionID);
}

void InitiatorApplication::fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID)
{
    orchestrator_->fromAdmin(message, sessionID);
}

void InitiatorApplication::toApp(FIX::Message& message, const FIX::SessionID& sessionID)
{
    orchestrator_->toApp(message, sessionID);
}

void InitiatorApplication::fromApp(const FIX::Message& message, const FIX::SessionID& sessionID)
{
    orchestrator_->onFromApp(message, sessionID);
}
