#include "SerialManager.hpp"

#define BUF_SIZE (1024)

SerialManager::SerialManager(std::shared_ptr<CommandManager> commandManager, esp_timer_handle_t *timerHandle) : commandManager(commandManager), timerHandle(timerHandle) {
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

  if (len) {
    esp_timer_stop(*timerHandle);
  }
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

    const auto result = this->commandManager->executeFromJson(std::string_view(reinterpret_cast<const char *>(this->data)));
    const auto resultMessage = result.getResult();
    usb_serial_jtag_write_bytes(resultMessage.c_str(), resultMessage.length(), 1000 / 20);
    esp_timer_start_once(*timerHandle, 30000000); // 30s
  }
}

void HandleSerialManagerTask(void *pvParameters)
{
  auto const serialManager = static_cast<SerialManager *>(pvParameters);

  while (true)
  {
    serialManager->try_receive();
  }
}