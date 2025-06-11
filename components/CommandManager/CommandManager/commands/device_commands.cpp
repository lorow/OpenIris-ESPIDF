#include "device_commands.hpp"

#include <cJSON.h>
#include <ProjectConfig.hpp>
#include "esp_timer.h"
#include <main_globals.hpp>

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

CommandResult startStreamingCommand() {
    activateStreaming(false); // Don't disable setup interfaces by default
    return CommandResult::getSuccessResult("Streaming started");
}

CommandResult switchModeCommand(std::shared_ptr<DependencyRegistry> registry, std::string_view jsonPayload) {
    const auto parsedJson = cJSON_Parse(jsonPayload.data());
    if (parsedJson == nullptr) {
        return CommandResult::getErrorResult("Invalid payload");
    }

    const auto modeObject = cJSON_GetObjectItem(parsedJson, "mode");
    if (modeObject == nullptr) {
        return CommandResult::getErrorResult("Invalid payload - missing mode");
    }

    const char* modeStr = modeObject->valuestring;
    StreamingMode newMode;
    
    ESP_LOGI("[DEVICE_COMMANDS]", "Switch mode command received with mode: %s", modeStr);
    
    if (strcmp(modeStr, "uvc") == 0 || strcmp(modeStr, "UVC") == 0) {
        newMode = StreamingMode::UVC;
    } else if (strcmp(modeStr, "wifi") == 0 || strcmp(modeStr, "WiFi") == 0 || strcmp(modeStr, "WIFI") == 0) {
        newMode = StreamingMode::WIFI;
    } else if (strcmp(modeStr, "auto") == 0 || strcmp(modeStr, "AUTO") == 0) {
        newMode = StreamingMode::AUTO;
    } else {
        return CommandResult::getErrorResult("Invalid mode - use 'uvc', 'wifi', or 'auto'");
    }

    const auto projectConfig = registry->resolve<ProjectConfig>(DependencyType::project_config);
    ESP_LOGI("[DEVICE_COMMANDS]", "Setting device mode to: %d", (int)newMode);
    projectConfig->setDeviceMode(newMode);
    
    cJSON_Delete(parsedJson);

    return CommandResult::getSuccessResult("Device mode switched, restart to apply");
}

CommandResult getDeviceModeCommand(std::shared_ptr<DependencyRegistry> registry) {
    const auto projectConfig = registry->resolve<ProjectConfig>(DependencyType::project_config);
    StreamingMode currentMode = projectConfig->getDeviceMode();
    
    const char* modeStr = "unknown";
    switch (currentMode) {
        case StreamingMode::UVC:
            modeStr = "UVC";
            break;
        case StreamingMode::WIFI:
            modeStr = "WiFi";
            break;
        case StreamingMode::AUTO:
            modeStr = "Auto";
            break;
    }
    
    char result[100];
    sprintf(result, "{\"mode\":\"%s\",\"value\":%d}", modeStr, (int)currentMode);
    
    return CommandResult::getSuccessResult(result);
}
