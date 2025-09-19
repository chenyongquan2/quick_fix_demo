#include "fix_message_converter.h"
#include "fix_custom.h"

#include <quickfix/Fields.h>
#include <quickfix/fix44/NewOrderSingle.h>
#include <quickfix/fix44/OrderCancelRequest.h>
#include <quickfix/fix44/OrderCancelReplaceRequest.h>
#include <quickfix/fix44/ExecutionReport.h>
#include <quickfix/fix44/OrderCancelReject.h>

#include <cstdio>

common::Order FixMessageConverter::parseNewOrderSingle(const FIX44::NewOrderSingle& msg)
{
    common::Order o;
    // Required
    FIX::ClOrdID clOrdId;
    msg.get(clOrdId);
    o.clOrdId = clOrdId.getValue();
    FIX::Side side;
    msg.get(side);
    o.side = side.getValue();
    FIX::Symbol symbol;
    msg.get(symbol);
    o.symbol = symbol.getValue();
    FIX::OrderQty qty;
    msg.get(qty);
    o.quantity = qty.getValue();
    FIX::OrdType ordType;
    msg.get(ordType);
    o.orderType = ordType.getValue();
    // Optional
    if (msg.isSetField(FIX::FIELD::Price)) {
        FIX::Price price;
        msg.get(price);
        o.price = price.getValue();
    }
    if (msg.isSetField(FIX::FIELD::TimeInForce)) {
        FIX::TimeInForce tif;
        msg.get(tif);
        o.timeInForce = tif.getValue();
    }
    if (msg.isSetField(FIX::FIELD::Account)) {
        FIX::Account account;
        msg.get(account);
        o.account = account.getValue();
    }
    return o;
}

common::Order FixMessageConverter::parseCancelRequest(const FIX44::OrderCancelRequest& msg, std::string& outOrigClOrdId)
{
    common::Order o;
    outOrigClOrdId.clear();
    if (msg.isSetField(FIX::FIELD::ClOrdID)) {
        FIX::ClOrdID clOrdId;
        msg.get(clOrdId);
        o.clOrdId = clOrdId.getValue();
    }
    if (msg.isSetField(FIX::FIELD::OrigClOrdID)) {
        FIX::OrigClOrdID orig;
        msg.get(orig);
        outOrigClOrdId = orig.getValue();
    }
    if (msg.isSetField(FIX::FIELD::Symbol)) {
        FIX::Symbol symbol;
        msg.get(symbol);
        o.symbol = symbol.getValue();
    }
    if (msg.isSetField(FIX::FIELD::Side)) {
        FIX::Side side;
        msg.get(side);
        o.side = side.getValue();
    }
    return o;
}

common::Order FixMessageConverter::parseReplaceRequest(
    const FIX44::OrderCancelReplaceRequest& msg, std::string& outOrigClOrdId)
{
    common::Order o;
    outOrigClOrdId.clear();
    if (msg.isSetField(FIX::FIELD::ClOrdID)) {
        FIX::ClOrdID clOrdId;
        msg.get(clOrdId);
        o.clOrdId = clOrdId.getValue();
    }
    if (msg.isSetField(FIX::FIELD::OrigClOrdID)) {
        FIX::OrigClOrdID orig;
        msg.get(orig);
        outOrigClOrdId = orig.getValue();
    }
    if (msg.isSetField(FIX::FIELD::Symbol)) {
        FIX::Symbol symbol;
        msg.get(symbol);
        o.symbol = symbol.getValue();
    }
    if (msg.isSetField(FIX::FIELD::Side)) {
        FIX::Side side;
        msg.get(side);
        o.side = side.getValue();
    }
    if (msg.isSetField(FIX::FIELD::OrderQty)) {
        FIX::OrderQty qty;
        msg.get(qty);
        o.quantity = qty.getValue();
    }
    if (msg.isSetField(FIX::FIELD::Price)) {
        FIX::Price price;
        msg.get(price);
        o.price = price.getValue();
    }
    if (msg.isSetField(FIX::FIELD::OrdType)) {
        FIX::OrdType ordType;
        msg.get(ordType);
        o.orderType = ordType.getValue();
    }
    return o;
}

FIX::Message FixMessageConverter::createExecutionReport(const common::Order& order, const std::string& execId,
    const std::string& execType, const std::string& ordStatus, const FIX::SessionID&)
{
    FIX44::ExecutionReport er(FIX::OrderID(order.orderId), FIX::ExecID(execId), FIX::ExecType(execType[0]),
        FIX::OrdStatus(ordStatus[0]), FIX::Side(order.side), FIX::LeavesQty(static_cast<double>(order.quantity)),
        FIX::CumQty(0), FIX::AvgPx(0));
    er.set(FIX::ClOrdID(order.clOrdId));
    if (!order.symbol.empty())
        er.set(FIX::Symbol(order.symbol));
    er.set(FIX::OrderQty(order.quantity));
    er.set(FIX::OrdType(order.orderType));
    if (order.price > 0)
        er.set(FIX::Price(order.price));
    er.setField(FIX::TransactTime());
    return er;
}

FIX::Message FixMessageConverter::createOrderReject(
    const std::string& clOrdId, const std::string& reason, const FIX::SessionID&)
{
    FIX44::OrderCancelReject rej;
    rej.set(FIX::ClOrdID(clOrdId));
    rej.setField(FIX::CxlRejResponseTo(FIX::CxlRejResponseTo_ORDER_CANCEL_REQUEST));
    rej.setField(FIX::CxlRejReason(99));
    rej.setField(FIX::Text(reason));
    rej.setField(FIX::TransactTime());
    return rej;
}

FIX::Message FixMessageConverter::createMarginUpdate(const common::MarginUpdate& update, const FIX::SessionID&)
{
    FIX::Message msg;
    msg.getHeader().setField(FIX::BeginString(FIX::BeginString_FIX44));
    msg.getHeader().setField(FIX::MsgType(fixcustom::kMsgTypeMarginUpdate));

    // 设置标准字段
    msg.setField(FIX::Account(update.account));
    msg.setField(FIX::Currency(update.currency));

    // 确保数值字段格式正确
    char buf1[64], buf2[64], buf3[64];
    std::snprintf(buf1, sizeof(buf1), "%.2f", update.marginValue);
    std::snprintf(buf2, sizeof(buf2), "%.2f", update.marginLevel);
    std::snprintf(buf3, sizeof(buf3), "%.2f", update.marginExcess);

    msg.setField(fixcustom::TAG_MARGIN_VALUE, std::string(buf1));
    msg.setField(fixcustom::TAG_MARGIN_LEVEL, std::string(buf2));
    msg.setField(fixcustom::TAG_MARGIN_EXCESS, std::string(buf3));

    // 添加时间戳
    msg.setField(FIX::TransactTime());

    return msg;
}
