#include "OpenIrisTasks.hpp"

void OpenIrisTasks::ScheduleRestart(int milliseconds)
{
  taskYIELD();
  const auto initialTime = Helpers::getTimeInMillis();
  while (Helpers::getTimeInMillis() - initialTime <= milliseconds)
  {
    continue;
  }

  esp_restart();
  taskYIELD();
}