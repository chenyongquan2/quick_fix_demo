#include <spdlog/spdlog.h>
#include <filesystem>

#include <quickfix/config.h>
#include <quickfix/FileStore.h>
#include <quickfix/Log.h>
#include <quickfix/SessionSettings.h>
#include <quickfix/FileLog.h>
#include <quickfix/SocketAcceptor.h>

#include "cc-common/utils.h"
#include "acceptor_application.h"

int main()
{
    try {
        std::string configuration_path
            = (std::filesystem::current_path() / "Config" / "black-arrow-common.ini").string();
        assert_file_exist(configuration_path);

        spdlog_configuration spdlog_c;
        spdlog_c.dynamic_flush_configuration_path_ = configuration_path;
        spdlog_c.async_ = true;
        init_spdlog(spdlog_c, "fix-acceptor");

        std::string fix_cfg_path = (std::filesystem::current_path() / "Config" / "fix-acceptor.cfg").string();
        // assert_file_exist(fix_cfg_path);

        AcceptorApplication application;
        FIX::SessionSettings settings(fix_cfg_path);
        FIX::FileStoreFactory storeFactory(settings);
        FIX::FileLogFactory logFactory(settings);
        FIX::SocketAcceptor acceptor(application, storeFactory, settings, logFactory);

        acceptor.start();
        SPDLOG_INFO("Acceptor started with settings: {}", fix_cfg_path);

        endless_wait();

        acceptor.stop();
        SPDLOG_INFO("Acceptor stopping...");

    } catch (const std::exception& e) {
        SPDLOG_ERROR("error:{}", e.what());
    }

    return -1;
}
