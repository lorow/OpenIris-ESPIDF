#include "device_commands.hpp"

CommandResult restartDeviceCommand::execute(std::string_view jsonPayload)
{
  // todo implement this: https://github.com/EyeTrackVR/OpenIris/blob/master/ESP/lib/src/tasks/tasks.cpp
  // OpenIrisTasks::ScheduleRestart(2000);
  return CommandResult::getSuccessResult("Device restarted");
}