#include "simple_commands.hpp"
#include "main_globals.hpp"
#include "esp_log.h"

static const char* TAG = "SimpleCommands";

CommandResult PingCommand()
{
  return CommandResult::getSuccessResult("pong");
};

CommandResult PauseCommand(const PausePayload& payload)
{
  ESP_LOGI(TAG, "Pause command received: %s", payload.pause ? "true" : "false");
  
  startupPaused = payload.pause;
  
  if (payload.pause) {
    ESP_LOGI(TAG, "Startup paused - device will remain in configuration mode");
    return CommandResult::getSuccessResult("Startup paused");
  } else {
    ESP_LOGI(TAG, "Startup resumed");
    return CommandResult::getSuccessResult("Startup resumed");
  }
};
