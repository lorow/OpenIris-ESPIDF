#include "SerialManager.hpp"
#include "esp_log.h"
#include "main_globals.hpp"

SerialManager::SerialManager(std::shared_ptr<CommandManager> commandManager, esp_timer_handle_t *timerHandle)
    : commandManager(commandManager), timerHandle(timerHandle)
{
  this->data = static_cast<uint8_t *>(malloc(BUF_SIZE));
  this->temp_data = static_cast<uint8_t *>(malloc(256));
}

// Function to notify that a command was received during startup
void SerialManager::notify_startup_command_received()
{
  setStartupCommandReceived(true);

  // Cancel the startup timer if it's still running
  if (timerHandle != nullptr && *timerHandle != nullptr)
  {
    esp_timer_stop(*timerHandle);
    esp_timer_delete(*timerHandle);
    *timerHandle = nullptr;
    ESP_LOGI("[MAIN]", "Startup timer cancelled, staying in heartbeat mode");
  }
}

// we can cancel this task once we're in cdc
void HandleSerialManagerTask(void *pvParameters)
{
  auto const serialManager = static_cast<SerialManager *>(pvParameters);
  while (true)
  {
    serialManager->try_receive();
  }
}
