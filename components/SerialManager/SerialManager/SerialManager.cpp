#include "SerialManager.hpp"
#include "esp_log.h"
#include "main_globals.hpp"
#include "tusb.h"

#define BUF_SIZE (1024)

SerialManager::SerialManager(std::shared_ptr<CommandManager> commandManager, esp_timer_handle_t *timerHandle, std::shared_ptr<ProjectConfig> deviceConfig)
    : commandManager(commandManager), timerHandle(timerHandle), deviceConfig(deviceConfig)
{
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

  // If driver is uninstalled or an error occurs, abort read gracefully
  if (len < 0)
  {
    return;
  }

  // since we've got something on the serial port
  // we gotta keep reading until we've got the whole message
  while (len > 0)
  {
    // Prevent buffer overflow
    if (current_position + len >= BUF_SIZE)
    {
      int copy_len = BUF_SIZE - 1 - current_position;
      if (copy_len > 0)
      {
        memcpy(this->data + current_position, this->temp_data, copy_len);
        current_position += copy_len;
      }
      // Drop the rest of the input to avoid overflow
      break;
    }
    memcpy(this->data + current_position, this->temp_data, (size_t)len);
    current_position += len;
    len = usb_serial_jtag_read_bytes(this->temp_data, 256, 1000 / 20);
    if (len < 0)
    {
      // Driver likely uninstalled during handover; stop processing this cycle
      break;
    }
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
    int written = usb_serial_jtag_write_bytes(resultMessage.c_str(), resultMessage.length(), 1000 / 20);
    (void)written; // ignore errors if driver already uninstalled
  }
}

// Function to notify that a command was received during startup
void SerialManager::notify_startup_command_received()
{
  setStartupCommandReceived(true);

  // Cancel the startup timer if it's still running
  if (timerHandle != nullptr)
  {
    esp_timer_stop(*timerHandle);
    esp_timer_delete(*timerHandle);
    timerHandle = nullptr;
    ESP_LOGI("[MAIN]", "Startup timer cancelled, staying in heartbeat mode");
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
  // Ignore return value; if the driver was uninstalled, this is a no-op
}

bool SerialManager::should_send_heartbeat()
{
  // Always send heartbeat during startup delay or if no WiFi configured

  // If startup timer is still running, always send heartbeat
  if (timerHandle != nullptr)
  {
    return true;
  }

  // If in heartbeat mode after startup, continue sending
  if (getStartupCommandReceived())
  {
    return true;
  }

  // Otherwise, only send if no WiFi credentials configured
  const auto wifiConfigs = deviceConfig->getWifiConfigs();
  return wifiConfigs.empty();
}

void SerialManager::shutdown()
{
  // Stop heartbeats; timer will be deleted by main if needed.
  // Uninstall the USB Serial JTAG driver to free the internal USB for TinyUSB.
  esp_err_t err = usb_serial_jtag_driver_uninstall();
  if (err == ESP_OK)
  {
    ESP_LOGI("[SERIAL]", "usb_serial_jtag driver uninstalled");
  }
  else if (err != ESP_ERR_INVALID_STATE)
  {
    ESP_LOGW("[SERIAL]", "usb_serial_jtag_driver_uninstall returned %s", esp_err_to_name(err));
  }
}

// we can cancel this task once we're in cdc
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
    if ((currentTime - lastHeartbeat) >= heartbeatInterval)
    {
      if (serialManager->should_send_heartbeat())
      {
        serialManager->send_heartbeat();
      }
      lastHeartbeat = currentTime;
    }
  }
}

void HandleCDCSerialManagerTask(void *pvParameters)
{
  auto const commandManager = static_cast<CommandManager *>(pvParameters);
  static char buffer[BUF_SIZE];
  auto idx = 0;

  cdc_command_packet_t packet;
  while (true)
  {
    if (xQueueReceive(cdcMessageQueue, &packet, portMAX_DELAY) == pdTRUE)
    {
      for (auto i = 0; i < packet.len; i++)
      {
        buffer[idx++] = packet.data[i];
        // if we're at the end of the buffer, try to process the command anyway
        // if we've got a new line, we've finished sending the commands, process them
        if (idx >= BUF_SIZE || buffer[idx - 1] == '\n' || buffer[idx - 1] == '\r')
        {
          buffer[idx - 1] = '\0';
          const auto result = commandManager->executeFromJson(std::string_view(reinterpret_cast<const char *>(buffer)));
          const auto resultMessage = result.getResult();
          tud_cdc_write(resultMessage.c_str(), resultMessage.length());
          tud_cdc_write_flush();
          idx = 0;
        }
      }
    }
  }
}

// tud_cdc_rx_cb is defined as TU_ATTR_WEAK so we can override it, we will be called back if we get some data
// but we don't want to do any processing here since we don't want to risk blocking
// grab the data and send it to a queue, a special task will process it and handle with the command manager
extern "C" void tud_cdc_rx_cb(uint8_t itf)
{
  // we can void the interface number
  (void)itf;
  cdc_command_packet_t packet;
  auto len = tud_cdc_available();

  if (len > 0)
  {
    auto read = tud_cdc_read(packet.data, sizeof(packet.data));
    if (read > 0)
    {
      // we should be safe here, given that the max buffer size is 64
      packet.len = static_cast<uint8_t>(read);
      xQueueSend(cdcMessageQueue, &packet, 1);
    }
  }
}

extern "C" void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
  (void)itf;
  (void)dtr;
  (void)rts;

  ESP_LOGI("[SERIAL]", "CDC line state changed: DTR=%d, RTS=%d", dtr, rts);
}

void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const *p_line_coding)
{
  (void)itf;
  ESP_LOGI("[SERIAL]", "CDC line coding: %" PRIu32 " bps, %d stop bits, %d parity, %d data bits",
           p_line_coding->bit_rate, p_line_coding->stop_bits,
           p_line_coding->parity, p_line_coding->data_bits);
}