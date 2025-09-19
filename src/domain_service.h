#pragma once

#include "common_types.h"

#include <atomic>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

class DomainService {
public:
    DomainService();
    ~DomainService();

    common::OrderResult processNewOrder(const common::Order& order);
    common::OrderResult processCancelOrder(const common::Order& order, const std::string& origClOrdId);
    common::OrderResult processReplaceOrder(const common::Order& order, const std::string& origClOrdId);

    std::optional<common::Order> findOrderByClOrdId(const std::string& clOrdId);
    std::vector<common::Order> getAllOrders();

    void setOrderStatusCallback(std::function<void(const common::Order&, const std::string&)> cb);
    void setMarginUpdateCallback(std::function<void(const common::MarginUpdate&)> cb);

    void startMarginUpdates();
    void stopMarginUpdates();

private:
    void storeOrder(const common::Order& order);
    bool updateOrderStatus(const std::string& clOrdId, const std::string& status);
    bool validateNewOrder(const common::Order& order) const;
    bool validateReplaceOrder(const common::Order& order) const;
    std::string genOrderId();
    std::string genExecId();
    void marginLoop();

private:
    std::vector<common::Order> orders_;
    std::mutex orders_mtx_;

    std::function<void(const common::Order&, const std::string&)> order_cb_;
    std::function<void(const common::MarginUpdate&)> margin_cb_;

    std::atomic<bool> margin_running_ { false };
    std::thread margin_thread_;
    std::atomic<unsigned long long> order_seq_ { 1 };
    std::atomic<unsigned long long> exec_seq_ { 1 };
};
