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

CommandResult restartDeviceCommand() {
    OpenIrisTasks::ScheduleRestart(2000);
    return CommandResult::getSuccessResult("Device restarted");
}
