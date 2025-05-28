#include "device_commands.hpp"

#include <cJSON.h>
#include <ProjectConfig.hpp>

// Implementation inspired by SummerSigh work, initial PR opened in openiris repo, adapted to this rewrite
CommandResult setDeviceModeCommand(std::shared_ptr<DependencyRegistry> registry, std::string_view jsonPayload) {
    const auto parsedJson = cJSON_Parse(jsonPayload.data());
    if (parsedJson == nullptr) {
        return CommandResult::getErrorResult("Invalid payload");
    }

    const auto modeObject = cJSON_GetObjectItem(parsedJson, "mode");
    if (modeObject == nullptr) {
        return CommandResult::getErrorResult("Invalid payload - missing mode");
    }

    const auto mode = modeObject->valueint;

    if (mode < 0 || mode > 2) {
        return CommandResult::getErrorResult("Invalid payload - unsupported mode");
    }

    const auto projectConfig = registry->resolve<ProjectConfig>(DependencyType::project_config);
    projectConfig->setDeviceMode(static_cast<StreamingMode>(mode));

    return CommandResult::getSuccessResult("Device mode set");
}

CommandResult updateOTACredentialsCommand(std::shared_ptr<DependencyRegistry> registry, std::string_view jsonPayload) {
    const auto parsedJson = cJSON_Parse(jsonPayload.data());

    if (parsedJson == nullptr) {
        return CommandResult::getErrorResult("Invalid payload");
    }

    const auto projectConfig = registry->resolve<ProjectConfig>(DependencyType::project_config);
    const auto oldDeviceConfig = projectConfig->getDeviceConfig();
    auto OTALogin = oldDeviceConfig.OTALogin;
    auto OTAPassword = oldDeviceConfig.OTAPassword;
    auto OTAPort = oldDeviceConfig.OTAPort;

    if (const auto OTALoginObject = cJSON_GetObjectItem(parsedJson, "login"); OTALoginObject != nullptr) {
        if (const auto newLogin = OTALoginObject->valuestring; strcmp(newLogin, "") != 0) {
            OTALogin = newLogin;
        }
    }

    if (const auto OTAPasswordObject = cJSON_GetObjectItem(parsedJson, "password"); OTAPasswordObject != nullptr) {
        OTAPassword = OTAPasswordObject->valuestring;
    }

    if (const auto OTAPortObject = cJSON_GetObjectItem(parsedJson, "port"); OTAPortObject != nullptr) {
        if (const auto newPort = OTAPortObject->valueint; newPort >= 82) {
            OTAPort = newPort;
        }
    }

    projectConfig->setDeviceConfig(OTALogin, OTAPassword, OTAPort);
    return CommandResult::getSuccessResult("OTA Config set");
}

CommandResult restartDeviceCommand() {
    OpenIrisTasks::ScheduleRestart(2000);
    return CommandResult::getSuccessResult("Device restarted");
}
