#include "CommandResult.hpp"
#include "OpenIrisTasks.hpp"
#include "DependencyRegistry.hpp"

CommandResult setDeviceModeCommand(std::shared_ptr<DependencyRegistry> registry, std::string_view jsonPayload);

CommandResult restartDeviceCommand();