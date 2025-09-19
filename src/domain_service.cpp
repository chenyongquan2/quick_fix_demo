#include "domain_service.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>

DomainService::DomainService() { }
DomainService::~DomainService() { stopMarginUpdates(); }

common::OrderResult DomainService::processNewOrder(const common::Order& order)
{
    if (!validateNewOrder(order))
        return { false, "", "", "Invalid order", order };
    common::Order o = order;

    // 为空的账户设置默认值
    if (o.account.empty()) {
        o.account = "DEFAULT_ACCOUNT";
    }

    o.orderId = genOrderId();
    o.status = "NEW";
    storeOrder(o);
    if (order_cb_)
        order_cb_(o, "NEW");
    return { true, o.orderId, genExecId(), "Order accepted", o };
}

common::OrderResult DomainService::processCancelOrder(const common::Order& order, const std::string& origClOrdId)
{
    auto ex = findOrderByClOrdId(origClOrdId);
    if (!ex)
        return { false, "", "", "Original order not found", order };
    if (ex->status != "NEW")
        return { false, "", "", "Order cannot be cancelled in current status", order };
    updateOrderStatus(origClOrdId, "CANCELED");
    common::Order o = *ex;
    o.status = "CANCELED";
    if (order_cb_)
        order_cb_(o, "CANCELED");
    return { true, o.orderId, genExecId(), "Order cancelled", o };
}

common::OrderResult DomainService::processReplaceOrder(const common::Order& order, const std::string& origClOrdId)
{
    auto ex = findOrderByClOrdId(origClOrdId);
    if (!ex)
        return { false, "", "", "Original order not found", order };
    if (ex->status != "NEW")
        return { false, "", "", "Order cannot be modified in current status", order };
    if (!validateReplaceOrder(order))
        return { false, "", "", "Invalid replace request", order };
    updateOrderStatus(origClOrdId, "REPLACED");
    common::Order o = *ex;
    o.status = "REPLACED";
    if (order_cb_)
        order_cb_(o, "REPLACED");
    return { true, o.orderId, genExecId(), "Order replaced", o };
}

std::optional<common::Order> DomainService::findOrderByClOrdId(const std::string& clOrdId)
{
    std::lock_guard<std::mutex> lk(orders_mtx_);
    auto it
        = std::find_if(orders_.begin(), orders_.end(), [&](const common::Order& o) { return o.clOrdId == clOrdId; });
    if (it == orders_.end())
        return std::nullopt;
    return *it;
}

std::vector<common::Order> DomainService::getAllOrders()
{
    std::lock_guard<std::mutex> lk(orders_mtx_);
    return orders_;
}

void DomainService::setOrderStatusCallback(std::function<void(const common::Order&, const std::string&)> cb)
{
    order_cb_ = std::move(cb);
}

void DomainService::setMarginUpdateCallback(std::function<void(const common::MarginUpdate&)> cb)
{
    margin_cb_ = std::move(cb);
}

void DomainService::startMarginUpdates()
{
    bool expected = false;
    if (!margin_running_.compare_exchange_strong(expected, true))
        return;
    margin_thread_ = std::thread([this] { marginLoop(); });
}

void DomainService::stopMarginUpdates()
{
    bool expected = true;
    if (!margin_running_.compare_exchange_strong(expected, false))
        return;
    if (margin_thread_.joinable())
        margin_thread_.join();
}

void DomainService::storeOrder(const common::Order& order)
{
    std::lock_guard<std::mutex> lk(orders_mtx_);
    orders_.push_back(order);
}

bool DomainService::updateOrderStatus(const std::string& clOrdId, const std::string& status)
{
    std::lock_guard<std::mutex> lk(orders_mtx_);
    auto it
        = std::find_if(orders_.begin(), orders_.end(), [&](const common::Order& o) { return o.clOrdId == clOrdId; });
    if (it == orders_.end())
        return false;
    it->status = status;
    return true;
}

bool DomainService::validateNewOrder(const common::Order& order) const
{
    // 检查数量
    if (order.quantity <= 0) {
        SPDLOG_WARN("Order validation failed: invalid quantity {}", order.quantity);
        return false;
    }

    // 检查价格（非市价单需要价格）
    if (order.orderType != '1' && order.price <= 0) {
        SPDLOG_WARN("Order validation failed: non-market order requires price");
        return false;
    }

    // 检查符号
    if (order.symbol.empty()) {
        SPDLOG_WARN("Order validation failed: empty symbol");
        return false;
    }

    // 账户可以为空，使用默认值
    // 放宽账户验证，允许空账户
    return true;
}

bool DomainService::validateReplaceOrder(const common::Order& order) const
{
    if (order.quantity <= 0)
        return false;
    if (order.orderType != '1' && order.price <= 0)
        return false;
    return true;
}

std::string DomainService::genOrderId() { return std::to_string(order_seq_++); }
std::string DomainService::genExecId() { return std::to_string(exec_seq_++); }

void DomainService::marginLoop()
{
    double margin = 100000.0;
    while (margin_running_) {
        for (int i = 0; i < 300 && margin_running_; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (!margin_running_)
            break;
        if (margin_cb_) {
            common::MarginUpdate mu;
            mu.account = "ACC-001";
            mu.marginValue = margin;
            mu.marginLevel = 30.0;
            mu.marginExcess = margin * 0.7;
            mu.currency = "USD";
            margin_cb_(mu);
        }
    }
}
