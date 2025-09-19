#include <spdlog/spdlog.h>
#include <filesystem>

#include <quickfix/config.h>
#include <quickfix/FileStore.h>
#include <quickfix/Log.h>
#include <quickfix/SessionSettings.h>
#include <quickfix/FileLog.h>
#include <quickfix/SocketInitiator.h>

#include "cc-common/utils.h"
#include "initiator_application.h"

int main()
{
    try {
        std::string configuration_path
            = (std::filesystem::current_path() / "Config" / "black-arrow-common.ini").string();
        assert_file_exist(configuration_path);

        spdlog_configuration spdlog_c;
        spdlog_c.dynamic_flush_configuration_path_ = configuration_path;
        spdlog_c.async_ = true;
        init_spdlog(spdlog_c, "fix-initiator");

        std::string fix_cfg_path = (std::filesystem::current_path() / "Config" / "fix-initiator.cfg").string();
        assert_file_exist(fix_cfg_path);

        InitiatorApplication application;
        FIX::SessionSettings settings(fix_cfg_path);
        FIX::FileStoreFactory storeFactory(settings);
        FIX::FileLogFactory logFactory(settings);
        FIX::SocketInitiator initiator(application, storeFactory, settings, logFactory);

        initiator.start();
        SPDLOG_INFO("Initiator started with settings: {}", fix_cfg_path);

        endless_wait();

        initiator.stop();
        SPDLOG_INFO("Initiator stopping...");

    } catch (const std::exception& e) {
        SPDLOG_ERROR("error:{}", e.what());
    }

    return -1;
}
