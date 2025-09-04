#include "CommandResult.hpp"
#include "ProjectConfig.hpp"
#include "OpenIrisTasks.hpp"
#include "DependencyRegistry.hpp"
#include "esp_timer.h"
#include "cJSON.h"
#include "main_globals.hpp"

#include <format>
#include <string>

CommandResult updateOTACredentialsCommand(std::shared_ptr<DependencyRegistry> registry, std::string_view jsonPayload);

CommandResult updateLEDDutyCycleCommand(std::shared_ptr<DependencyRegistry> registry, std::string_view jsonPayload);
CommandResult getLEDDutyCycleCommand(std::shared_ptr<DependencyRegistry> registry);

CommandResult restartDeviceCommand();

CommandResult startStreamingCommand();

CommandResult switchModeCommand(std::shared_ptr<DependencyRegistry> registry, std::string_view jsonPayload);

CommandResult getDeviceModeCommand(std::shared_ptr<DependencyRegistry> registry);

CommandResult getSerialNumberCommand(std::shared_ptr<DependencyRegistry> registry);

// Monitoring
CommandResult getLEDCurrentCommand(std::shared_ptr<DependencyRegistry> registry);