#include "SerialManager.hpp"
#include "esp_log.h"
#include "main_globals.hpp"
#include "driver/uart.h"

void SerialManager::setup()
{
  // usb_serial_jtag_driver_config_t usb_serial_jtag_config;
  // usb_serial_jtag_config.rx_buffer_size = BUF_SIZE;
  // usb_serial_jtag_config.tx_buffer_size = BUF_SIZE;
  // usb_serial_jtag_driver_install(&usb_serial_jtag_config);
}

void SerialManager::try_receive()
{
}

void SerialManager::shutdown()
{
  // Stop heartbeats; timer will be deleted by main if needed.
  // Uninstall the USB Serial JTAG driver to free the internal USB for TinyUSB.
  // esp_err_t err = usb_serial_jtag_driver_uninstall();
  // if (err == ESP_OK)
  // {
  //   ESP_LOGI("[SERIAL]", "usb_serial_jtag driver uninstalled");
  // }
  // else if (err != ESP_ERR_INVALID_STATE)
  // {
  //   ESP_LOGW("[SERIAL]", "usb_serial_jtag_driver_uninstall returned %s", esp_err_to_name(err));
  // }
}
