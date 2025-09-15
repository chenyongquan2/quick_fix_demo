#include <quickfix/Application.h>
#include <quickfix/MessageCracker.h>
#include <quickfix/Session.h>
#include <quickfix/SessionSettings.h>
#include <quickfix/FileStore.h>
#include <quickfix/FileLog.h>
#include <quickfix/SocketInitiator.h>
#include <quickfix/fix44/NewOrderSingle.h>
#include <quickfix/fix44/ExecutionReport.h>
#include <quickfix/fix44/OrderCancelReject.h>

#include <chrono>
#include <filesystem>
#include <csignal>
#include <iostream>
#include <random>
#include <string>
#include <thread>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/hourly_file_sink.h>

class ClientApplication : public FIX::Application, public FIX::MessageCracker {
public:
    void onCreate(const FIX::SessionID& sessionID) override {
        SPDLOG_INFO("[INIT] onCreate: {}", sessionID.toString());
    }

    void onLogon(const FIX::SessionID& sessionID) override {
        SPDLOG_INFO("[INIT] onLogon:  {}", sessionID.toString());
        session_ = sessionID;
        // Send a sample NewOrderSingle to be echoed back
        trySendNewOrder();
    }

    void onLogout(const FIX::SessionID& sessionID) override {
        SPDLOG_INFO("[INIT] onLogout: {}", sessionID.toString());
    }

    void toAdmin(FIX::Message& message, const FIX::SessionID& sessionID) override {
        SPDLOG_DEBUG("[INIT] toAdmin:  {}", message.toString());
    }

    void fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID) override {
        SPDLOG_DEBUG("[INIT] fromAdmin:{}", message.toString());
    }

    void toApp(FIX::Message& message, const FIX::SessionID& sessionID) override
        /* throw(FIX::DoNotSend) */ {
        SPDLOG_INFO("[INIT] toApp:    {}", message.toString());
    }

    void fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) override
        /* throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) */ {
        SPDLOG_INFO("[INIT] fromApp:  {}", message.toString());
        crack(message, sessionID);
    }

    // Handle ER(8)
    void onMessage(const FIX44::ExecutionReport& er, const FIX::SessionID& sessionID) override {
        try {
            FIX::ExecType execType; er.get(execType);
            FIX::OrdStatus ordStatus; er.get(ordStatus);
            FIX::ClOrdID clOrdId; if (er.isSetField(clOrdId)) er.get(clOrdId); else clOrdId = FIX::ClOrdID("");
            SPDLOG_INFO("[INIT] ER: ClOrdID={}, ExecType={}, OrdStatus={}",
                        clOrdId.getValue(), execType.getValue(), ordStatus.getValue());
        } catch (const std::exception& ex) {
            SPDLOG_ERROR("[INIT] onMessage(ER) error: {}", ex.what());
        }
    }

    // Handle 9
    void onMessage(const FIX44::OrderCancelReject& rej, const FIX::SessionID& sessionID) override {
        try {
            FIX::ClOrdID clOrdId; if (rej.isSetField(clOrdId)) rej.get(clOrdId); else clOrdId = FIX::ClOrdID("");
            FIX::Text text; if (rej.isSetField(text)) rej.get(text); else text = FIX::Text("");
            SPDLOG_INFO("[INIT] CxlReject: ClOrdID={}, Text={}", clOrdId.getValue(), text.getValue());
        } catch (const std::exception& ex) {
            SPDLOG_ERROR("[INIT] onMessage(9) error: {}", ex.what());
        }
    }

private:
    void trySendNewOrder() {
        try {
            FIX::Session* session = FIX::Session::lookupSession(session_);
            if (!session) return;

            FIX44::NewOrderSingle nos;
            nos.setField(FIX::ClOrdID(generateClOrdId()));
            nos.setField(FIX::Side(FIX::Side_BUY));
            nos.setField(FIX::TransactTime());
            nos.setField(FIX::OrdType(FIX::OrdType_MARKET));
            nos.setField(FIX::Symbol("ECHO"));
            nos.setField(FIX::OrderQty(1));

            FIX::Session::sendToTarget(nos, session_);
        } catch (const std::exception& ex) {
            SPDLOG_ERROR("[INIT] send error: {}", ex.what());
        }
    }

    static std::string generateClOrdId() {
        static std::mt19937_64 rng{std::random_device{}()};
        return std::to_string(rng());
    }

    FIX::SessionID session_;
};

static bool g_running = true;
static void handle_signal(int) { g_running = false; }

int main(int argc, char** argv) {
    const std::string settingsPath = (argc > 1) ? argv[1] : std::string("config/initiator.cfg");
    try {
        // setup spdlog hourly file sink
        {
            namespace fs = std::filesystem;
            const fs::path dir = fs::path("log") / "spdlog";
            std::error_code ec; fs::create_directories(dir, ec);
        }
        auto logger = spdlog::hourly_logger_mt("initiator", "log/spdlog/initiator.log", 0, 0);
        logger->set_level(spdlog::level::info);
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] [%s:%#] %v");
        spdlog::set_default_logger(logger);
        spdlog::flush_on(spdlog::level::info);

        ClientApplication application;
        FIX::SessionSettings settings(settingsPath);
        FIX::FileStoreFactory storeFactory(settings);
        FIX::FileLogFactory logFactory(settings);
        FIX::SocketInitiator initiator(application, storeFactory, settings, logFactory);

        std::signal(SIGINT, handle_signal);
        std::signal(SIGTERM, handle_signal);

        initiator.start();
        SPDLOG_INFO("Initiator started with settings: {}", settingsPath);
        SPDLOG_INFO("[INIT] Started. Press Ctrl+C to stop.");
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        SPDLOG_INFO("[INIT] Stopping...");
        SPDLOG_INFO("Initiator stopping...");
        initiator.stop();
    } catch (const std::exception& ex) {
        SPDLOG_ERROR("[INIT] Fatal error: {}", ex.what());
        try { SPDLOG_ERROR("Fatal error: {}", ex.what()); } catch (...) {}
        return 1;
    }
    return 0;
}


