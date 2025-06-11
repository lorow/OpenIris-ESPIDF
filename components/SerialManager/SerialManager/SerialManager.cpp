#include "SerialManager.hpp"
#include "esp_log.h"
#include "main_globals.hpp"

#define BUF_SIZE (1024)

SerialManager::SerialManager(std::shared_ptr<CommandManager> commandManager, esp_timer_handle_t *timerHandle, std::shared_ptr<ProjectConfig> deviceConfig) 
  : commandManager(commandManager), timerHandle(timerHandle), deviceConfig(deviceConfig) {
  this->data = static_cast<uint8_t *>(malloc(BUF_SIZE));
  this->temp_data = static_cast<uint8_t *>(malloc(256));
}

void SerialManager::setup()
{
  usb_serial_jtag_driver_config_t usb_serial_jtag_config;
  usb_serial_jtag_config.rx_buffer_size = BUF_SIZE;
  usb_serial_jtag_config.tx_buffer_size = BUF_SIZE;
  usb_serial_jtag_driver_install(&usb_serial_jtag_config);
}

void SerialManager::try_receive()
{
  int current_position = 0;
  int len = usb_serial_jtag_read_bytes(this->temp_data, 256, 1000 / 20);

  // since we've got something on the serial port
  // we gotta keep reading until we've got the whole message
  while (len)
  {
    memcpy(this->data + current_position, this->temp_data, len);
    current_position += len;
    len = usb_serial_jtag_read_bytes(this->temp_data, 256, 1000 / 20);
  }

  if (current_position)
  {
    // once we're done, we can terminate the string and reset the counter
    data[current_position] = '\0';
    current_position = 0;

    // Notify main that a command was received during startup
    notify_startup_command_received();
    
    const auto result = this->commandManager->executeFromJson(std::string_view(reinterpret_cast<const char *>(this->data)));
    const auto resultMessage = result.getResult();
    usb_serial_jtag_write_bytes(resultMessage.c_str(), resultMessage.length(), 1000 / 20);
  }
}

void SerialManager::send_heartbeat()
{
  // Get the MAC address as unique identifier
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  
  // Format as serial number string
  char serial_number[18];
  sprintf(serial_number, "%02X%02X%02X%02X%02X%02X", 
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  
  // Create heartbeat JSON with serial number
  char heartbeat[128];
  sprintf(heartbeat, "{\"heartbeat\":\"openiris_setup_mode\",\"serial\":\"%s\"}\n", serial_number);
  
  usb_serial_jtag_write_bytes(heartbeat, strlen(heartbeat), 1000 / 20);
}

bool SerialManager::should_send_heartbeat()
{
  // Always send heartbeat during startup delay or if no WiFi configured
  extern bool startupCommandReceived;
  extern esp_timer_handle_t startupTimerHandle;
  
  // If startup timer is still running, always send heartbeat
  if (startupTimerHandle != nullptr) {
    return true;
  }
  
  // If in heartbeat mode after startup, continue sending
  if (startupCommandReceived) {
    return true;
  }
  
  // Otherwise, only send if no WiFi credentials configured
  const auto wifiConfigs = deviceConfig->getWifiConfigs();
  return wifiConfigs.empty();
}

void HandleSerialManagerTask(void *pvParameters)
{
  auto const serialManager = static_cast<SerialManager *>(pvParameters);
  TickType_t lastHeartbeat = xTaskGetTickCount();
  const TickType_t heartbeatInterval = pdMS_TO_TICKS(2000); // 2 second heartbeat

  while (true)
  {
    serialManager->try_receive();
    
    // Send heartbeat every 2 seconds, but only if no WiFi credentials are set
    TickType_t currentTime = xTaskGetTickCount();
    if ((currentTime - lastHeartbeat) >= heartbeatInterval) {
      if (serialManager->should_send_heartbeat()) {
        serialManager->send_heartbeat();
      }
      lastHeartbeat = currentTime;
    }
    
    vTaskDelay(pdMS_TO_TICKS(50)); // Small delay to prevent busy waiting
  }
}