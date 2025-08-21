#include "device_commands.hpp"
#include "LEDManager.hpp"

// Implementation inspired by SummerSigh work, initial PR opened in openiris repo, adapted to this rewrite
CommandResult setDeviceModeCommand(std::shared_ptr<DependencyRegistry> registry, std::string_view jsonPayload)
{
    const auto parsedJson = cJSON_Parse(jsonPayload.data());
    if (parsedJson == nullptr)
    {
        return CommandResult::getErrorResult("Invalid payload");
    }

    const auto modeObject = cJSON_GetObjectItem(parsedJson, "mode");
    if (modeObject == nullptr)
    {
        return CommandResult::getErrorResult("Invalid payload - missing mode");
    }

    const auto mode = modeObject->valueint;

    if (mode < 0 || mode > 2)
    {
        return CommandResult::getErrorResult("Invalid payload - unsupported mode");
    }

    const auto projectConfig = registry->resolve<ProjectConfig>(DependencyType::project_config);
    projectConfig->setDeviceMode(static_cast<StreamingMode>(mode));

    cJSON_Delete(parsedJson);

    return CommandResult::getSuccessResult("Device mode set");
}

CommandResult updateOTACredentialsCommand(std::shared_ptr<DependencyRegistry> registry, std::string_view jsonPayload)
{
    const auto parsedJson = cJSON_Parse(jsonPayload.data());

    if (parsedJson == nullptr)
    {
        return CommandResult::getErrorResult("Invalid payload");
    }

    const auto projectConfig = registry->resolve<ProjectConfig>(DependencyType::project_config);
    const auto oldDeviceConfig = projectConfig->getDeviceConfig();
    auto OTALogin = oldDeviceConfig.OTALogin;
    auto OTAPassword = oldDeviceConfig.OTAPassword;
    auto OTAPort = oldDeviceConfig.OTAPort;

    if (const auto OTALoginObject = cJSON_GetObjectItem(parsedJson, "login"); OTALoginObject != nullptr)
    {
        if (const auto newLogin = OTALoginObject->valuestring; strcmp(newLogin, "") != 0)
        {
            OTALogin = newLogin;
        }
    }

    if (const auto OTAPasswordObject = cJSON_GetObjectItem(parsedJson, "password"); OTAPasswordObject != nullptr)
    {
        OTAPassword = OTAPasswordObject->valuestring;
    }

    if (const auto OTAPortObject = cJSON_GetObjectItem(parsedJson, "port"); OTAPortObject != nullptr)
    {
        if (const auto newPort = OTAPortObject->valueint; newPort >= 82)
        {
            OTAPort = newPort;
        }
    }

    cJSON_Delete(parsedJson);

    projectConfig->setOTAConfig(OTALogin, OTAPassword, OTAPort);
    return CommandResult::getSuccessResult("OTA Config set");
}

CommandResult updateLEDDutyCycleCommand(std::shared_ptr<DependencyRegistry> registry, std::string_view jsonPayload)
{
    const auto parsedJson = cJSON_Parse(jsonPayload.data());
    if (parsedJson == nullptr)
    {
        return CommandResult::getErrorResult("Invalid payload");
    }

    const auto dutyCycleObject = cJSON_GetObjectItem(parsedJson, "dutyCycle");
    if (dutyCycleObject == nullptr)
    {
        return CommandResult::getErrorResult("Invalid payload - missing dutyCycle");
    }

    const auto dutyCycle = dutyCycleObject->valueint;

    if (dutyCycle < 0 || dutyCycle > 100)
    {
        return CommandResult::getErrorResult("Invalid payload - unsupported dutyCycle");
    }

    const auto projectConfig = registry->resolve<ProjectConfig>(DependencyType::project_config);
    projectConfig->setLEDDUtyCycleConfig(dutyCycle);

    // Try to apply the change live via LEDManager if available
    auto ledMgr = registry->resolve<LEDManager>(DependencyType::led_manager);
    if (ledMgr)
    {
        ledMgr->setExternalLEDDutyCycle(static_cast<uint8_t>(dutyCycle));
    }

    cJSON_Delete(parsedJson);

    return CommandResult::getSuccessResult("LED duty cycle set");
}

CommandResult restartDeviceCommand()
{
    OpenIrisTasks::ScheduleRestart(2000);
    return CommandResult::getSuccessResult("Device restarted");
}

CommandResult getLEDDutyCycleCommand(std::shared_ptr<DependencyRegistry> registry)
{
    const auto projectConfig = registry->resolve<ProjectConfig>(DependencyType::project_config);
    const auto deviceCfg = projectConfig->getDeviceConfig();
    int duty = deviceCfg.led_external_pwm_duty_cycle;
    auto result = std::format("{{ \"led_external_pwm_duty_cycle\": {} }}", duty);
    return CommandResult::getSuccessResult(result);
}

CommandResult startStreamingCommand()
{
    activateStreaming(false); // Don't disable setup interfaces by default
    return CommandResult::getSuccessResult("Streaming started");
}

CommandResult switchModeCommand(std::shared_ptr<DependencyRegistry> registry, std::string_view jsonPayload)
{
    const auto parsedJson = cJSON_Parse(jsonPayload.data());
    if (parsedJson == nullptr)
    {
        return CommandResult::getErrorResult("Invalid payload");
    }

    const auto modeObject = cJSON_GetObjectItem(parsedJson, "mode");
    if (modeObject == nullptr)
    {
        return CommandResult::getErrorResult("Invalid payload - missing mode");
    }

    const char *modeStr = modeObject->valuestring;
    StreamingMode newMode;

    ESP_LOGI("[DEVICE_COMMANDS]", "Switch mode command received with mode: %s", modeStr);

    if (strcmp(modeStr, "uvc") == 0)
    {
        newMode = StreamingMode::UVC;
    }
    else if (strcmp(modeStr, "wifi") == 0)
    {
        newMode = StreamingMode::WIFI;
    }
    else if (strcmp(modeStr, "auto") == 0)
    {
        newMode = StreamingMode::AUTO;
    }
    else
    {
        return CommandResult::getErrorResult("Invalid mode - use 'uvc', 'wifi', or 'auto'");
    }

    const auto projectConfig = registry->resolve<ProjectConfig>(DependencyType::project_config);
    ESP_LOGI("[DEVICE_COMMANDS]", "Setting device mode to: %d", (int)newMode);
    projectConfig->setDeviceMode(newMode);

    cJSON_Delete(parsedJson);

    return CommandResult::getSuccessResult("Device mode switched, restart to apply");
}

CommandResult getDeviceModeCommand(std::shared_ptr<DependencyRegistry> registry)
{
    const auto projectConfig = registry->resolve<ProjectConfig>(DependencyType::project_config);
    StreamingMode currentMode = projectConfig->getDeviceMode();

    const char *modeStr = "unknown";
    switch (currentMode)
    {
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

    auto result = std::format("{{ \"mode\": \"{}\", \"value\": {} }}", modeStr, static_cast<int>(currentMode));
    return CommandResult::getSuccessResult(result);
}
