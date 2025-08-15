#include "simple_commands.hpp"

static const char *TAG = "SimpleCommands";

CommandResult PingCommand()
{
  return CommandResult::getSuccessResult("pong");
};

CommandResult PauseCommand(std::string_view jsonPayload)
{
  PausePayload payload;
  // pause by default if this command gets executed, even if the payload was invalid
  payload.pause = true;

  cJSON *root = cJSON_Parse(std::string(jsonPayload).c_str());
  if (root)
  {
    cJSON *pauseItem = cJSON_GetObjectItem(root, "pause");
    if (pauseItem && cJSON_IsBool(pauseItem))
    {
      payload.pause = cJSON_IsTrue(pauseItem);
    }
    cJSON_Delete(root);
  }

  ESP_LOGI(TAG, "Pause command received: %s", payload.pause ? "true" : "false");

  setStartupPaused(payload.pause);
  if (payload.pause)
  {
    ESP_LOGI(TAG, "Startup paused - device will remain in configuration mode");
    return CommandResult::getSuccessResult("Startup paused");
  }
  else
  {
    ESP_LOGI(TAG, "Startup resumed");
    return CommandResult::getSuccessResult("Startup resumed");
  }
};
