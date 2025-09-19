#pragma once

#include <quickfix/Message.h>
#include <quickfix/Session.h>

#include <optional>

// Interface to abstract QuickFIX message sending for testability.
class FixSender {
public:
    virtual ~FixSender() = default;
    virtual bool sendToTarget(FIX::Message& message, const FIX::SessionID& session_id) = 0;
};

// Default implementation that wraps FIX::Session::sendToTarget.
class QuickFixSender final : public FixSender {
public:
    bool sendToTarget(FIX::Message& message, const FIX::SessionID& session_id) override
    {
        try {
            return FIX::Session::sendToTarget(message, session_id);
        } catch (...) {
            return false;
        }
    }
};
