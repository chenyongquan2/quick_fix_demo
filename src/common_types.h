#pragma once

#include <string>

namespace common {

struct Order {
    std::string orderId;
    std::string clOrdId;
    std::string symbol;
    char side { '1' }; // FIX Side (1=Buy,2=Sell)
    double quantity { 0.0 };
    char orderType { '1' }; // FIX OrdType (1=Market,2=Limit,...)
    double price { 0.0 };
    char timeInForce { '0' }; // FIX TIF (0=Day,...)
    std::string account;
    std::string status; // NEW/CANCELED/REPLACED
};

struct OrderResult {
    bool success { false };
    std::string orderId;
    std::string execId;
    std::string message;
    Order updatedOrder; // echo/back
};

struct MarginUpdate {
    std::string account;
    double marginValue { 0.0 };
    double marginLevel { 0.0 };
    double marginExcess { 0.0 };
    std::string currency;
};

} // namespace common
