#pragma once

#include "common_types.h"

#include <quickfix/Message.h>
#include <quickfix/SessionID.h>
// Ensure FIX44 strong types are visible to any translation unit including this header
#include <quickfix/fix44/NewOrderSingle.h>
#include <quickfix/fix44/OrderCancelRequest.h>
#include <quickfix/fix44/OrderCancelReplaceRequest.h>

class FixMessageConverter {
public:
    // Overloads for FIX44 strong types to avoid dynamic_cast path issues
    static common::Order parseNewOrderSingle(const FIX44::NewOrderSingle& msg);
    static common::Order parseCancelRequest(const FIX44::OrderCancelRequest& msg, std::string& outOrigClOrdId);
    static common::Order parseReplaceRequest(const FIX44::OrderCancelReplaceRequest& msg, std::string& outOrigClOrdId);

    static FIX::Message createExecutionReport(const common::Order& order, const std::string& execId,
        const std::string& execType, const std::string& ordStatus, const FIX::SessionID& sessionID);

    static FIX::Message createOrderReject(
        const std::string& clOrdId, const std::string& reason, const FIX::SessionID& sessionID);

    static FIX::Message createMarginUpdate(const common::MarginUpdate& update, const FIX::SessionID& sessionID);
};
