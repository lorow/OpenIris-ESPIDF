#include "device_commands.hpp"

CommandResult restartDeviceCommand()
{
  // todo implement this: https://github.com/EyeTrackVR/OpenIris/blob/master/ESP/lib/src/tasks/tasks.cpp
  // OpenIrisTasks::ScheduleRestart(2000);
  return CommandResult::getSuccessResult("Device restarted");
}