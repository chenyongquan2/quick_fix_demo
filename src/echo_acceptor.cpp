#include <quickfix/Application.h>
#include <quickfix/MessageCracker.h>
#include <quickfix/Session.h>
#include <quickfix/SessionSettings.h>
#include <quickfix/FileStore.h>
#include <quickfix/FileLog.h>
#include <quickfix/SocketAcceptor.h>

#include <quickfix/fix44/NewOrderSingle.h>
#include <quickfix/fix44/OrderCancelRequest.h>
#include <quickfix/fix44/OrderCancelReplaceRequest.h>
#include <quickfix/fix44/ExecutionReport.h>
#include <quickfix/fix44/OrderCancelReject.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/hourly_file_sink.h>

#include <csignal>
#include <filesystem>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

class EchoApplication : public FIX::Application, public FIX::MessageCracker {
public:
    void onCreate(const FIX::SessionID& sessionID) override {
        SPDLOG_INFO("[ACCEPTOR] onCreate: {}", sessionID.toString());
    }

    void onLogon(const FIX::SessionID& sessionID) override {
        SPDLOG_INFO("[ACCEPTOR] onLogon: {}", sessionID.toString());
    }

    void onLogout(const FIX::SessionID& sessionID) override {
        SPDLOG_INFO("[ACCEPTOR] onLogout: {}", sessionID.toString());
    }

    void toAdmin(FIX::Message& message, const FIX::SessionID& sessionID) override {
        SPDLOG_DEBUG("[ACCEPTOR] toAdmin: {}", message.toString());
    }

    void fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID) override {
        SPDLOG_DEBUG("[ACCEPTOR] fromAdmin: {}", message.toString());
    }

    void toApp(FIX::Message& message, const FIX::SessionID& sessionID) override
        /* throw(FIX::DoNotSend) */ {
        SPDLOG_INFO("[ACCEPTOR] toApp:   {}", message.toString());
    }

    void fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) override
        /* throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) */ {
        SPDLOG_INFO("[ACCEPTOR] fromApp: {}", message.toString());
        crack(message, sessionID);
    }

    // Strongly-typed handlers for FIX 4.4
    void onMessage(const FIX44::NewOrderSingle& nos, const FIX::SessionID& sessionID) override {
        try {
            // Extract minimal fields
            FIX::ClOrdID clOrdId; nos.get(clOrdId);
            FIX::Side side; nos.get(side);
            FIX::Symbol symbol; if (nos.isSetField(symbol)) nos.get(symbol); else symbol = FIX::Symbol("N/A");
            FIX::OrderQty orderQty; if (nos.isSetField(orderQty)) nos.get(orderQty); else orderQty = FIX::OrderQty(0);

            // Build basic ExecutionReport (New)
            FIX44::ExecutionReport er(
                FIX::OrderID(generateId()),
                FIX::ExecID(generateId()),
                FIX::ExecType(FIX::ExecType_NEW),
                FIX::OrdStatus(FIX::OrdStatus_NEW),
                side,
                FIX::LeavesQty(orderQty.getValue()),
                FIX::CumQty(0),
                FIX::AvgPx(0)
            );
            er.set(clOrdId);
            er.set(symbol);
            er.setField(FIX::TransactTime());

            FIX::Session::sendToTarget(er, sessionID);
        } catch (const std::exception& ex) {
            SPDLOG_ERROR("[ACCEPTOR] onMessage(NOS) error: {}", ex.what());
            sendBusinessReject(sessionID, "NOS error");
        }
    }

    void onMessage(const FIX44::OrderCancelRequest& ocr, const FIX::SessionID& sessionID) override {
        try {
            // Skeleton: reject by default (no order book in demo)
            FIX::ClOrdID clOrdId; if (ocr.isSetField(clOrdId)) ocr.get(clOrdId); else clOrdId = FIX::ClOrdID(generateId());
            FIX44::OrderCancelReject rej;
            rej.set(clOrdId);
            rej.setField(FIX::CxlRejResponseTo(FIX::CxlRejResponseTo_ORDER_CANCEL_REQUEST));
            rej.setField(FIX::CxlRejReason(0)); // Too late to cancel (as placeholder)
            rej.setField(FIX::Text("Cancel not supported in demo"));
            rej.setField(FIX::TransactTime());
            FIX::Session::sendToTarget(rej, sessionID);
        } catch (const std::exception& ex) {
            SPDLOG_ERROR("[ACCEPTOR] onMessage(CxlReq) error: {}", ex.what());
            sendBusinessReject(sessionID, "OCR error");
        }
    }

    void onMessage(const FIX44::OrderCancelReplaceRequest& ocrr, const FIX::SessionID& sessionID) override {
        try {
            FIX::ClOrdID clOrdId; if (ocrr.isSetField(clOrdId)) ocrr.get(clOrdId); else clOrdId = FIX::ClOrdID(generateId());
            FIX44::OrderCancelReject rej;
            rej.set(clOrdId);
            rej.setField(FIX::CxlRejResponseTo(FIX::CxlRejResponseTo_ORDER_CANCEL_REPLACE_REQUEST));
            rej.setField(FIX::CxlRejReason(0));
            rej.setField(FIX::Text("Replace not supported in demo"));
            rej.setField(FIX::TransactTime());
            FIX::Session::sendToTarget(rej, sessionID);
        } catch (const std::exception& ex) {
            SPDLOG_ERROR("[ACCEPTOR] onMessage(OCrr) error: {}", ex.what());
            sendBusinessReject(sessionID, "OCRR error");
        }
    }

private:
    static std::string generateId() {
        static std::atomic<unsigned long long> counter{1};
        auto v = counter.fetch_add(1, std::memory_order_relaxed);
        return std::to_string(v);
    }

    static void sendBusinessReject(const FIX::SessionID& sessionID, const std::string& text) {
        try {
            // Use generic Business Message Reject (j) if needed in future; for now, use session Reject(3)?
            // Here we simply log; full BMR requires RefMsgType, BusinessRejectReason, etc.
            (void)sessionID; (void)text;
        } catch (...) {
        }
    }
};

static bool g_running = true;

static void handle_signal(int) {
    g_running = false;
}

int main(int argc, char** argv) {
    const std::string settingsPath = (argc > 1) ? argv[1] : std::string("config/acceptor.cfg");

    try {
        // setup spdlog hourly file sink
        {
            namespace fs = std::filesystem;
            const fs::path dir = fs::path("log") / "spdlog";
            std::error_code ec; fs::create_directories(dir, ec);
        }
        auto logger = spdlog::hourly_logger_mt("acceptor", "log/spdlog/acceptor.log", 0, 0);
        logger->set_level(spdlog::level::trace);
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");
        spdlog::set_default_logger(logger);
        spdlog::flush_on(spdlog::level::info);

        EchoApplication application;
        FIX::SessionSettings settings(settingsPath);
        FIX::FileStoreFactory storeFactory(settings);
        FIX::FileLogFactory logFactory(settings);
        FIX::SocketAcceptor acceptor(application, storeFactory, settings, logFactory);

        std::signal(SIGINT, handle_signal);
        std::signal(SIGTERM, handle_signal);

        acceptor.start();
        SPDLOG_INFO("Acceptor started with settings: {}", settingsPath);
        SPDLOG_INFO("[ACCEPTOR] Started. Press Ctrl+C to stop.");
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        SPDLOG_INFO("[ACCEPTOR] Stopping...");
        SPDLOG_INFO("Acceptor stopping...");
        acceptor.stop();
    } catch (const std::exception& ex) {
        SPDLOG_ERROR("[ACCEPTOR] Fatal error: {}", ex.what());
        try { SPDLOG_ERROR("Fatal error: {}", ex.what()); } catch (...) {}
        return 1;
    }
    return 0;
}


