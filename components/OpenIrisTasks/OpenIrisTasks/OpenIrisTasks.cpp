#include "OpenIrisTasks.hpp"

void restart_the_board(void *arg) {
  esp_restart();
}

void OpenIrisTasks::ScheduleRestart(const int milliseconds)
{
  esp_timer_handle_t timerHandle;
  constexpr esp_timer_create_args_t args = {
    .callback = &restart_the_board,
    .arg = nullptr,
    .name = "restartBoard"};

  if (const auto result = esp_timer_create(&args, &timerHandle); result == ESP_OK)
  {
    esp_timer_start_once(timerHandle, milliseconds);
  }
}