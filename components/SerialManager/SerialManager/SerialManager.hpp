#pragma once
#ifndef SERIALMANAGER_HPP
#define SERIALMANAGER_HPP

#include <stdio.h>
#include <string>
#include <memory>
#include <CommandManager.hpp>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/usb_serial_jtag.h"
#include "esp_vfs_usb_serial_jtag.h"
#include "esp_vfs_dev.h"

class SerialManager
{
public:
  SerialManager(std::shared_ptr<CommandManager> commandManager);
  void setup();
  void try_receive();

private:
  QueueHandle_t serialQueue;
  std::shared_ptr<CommandManager> commandManager;
  uint8_t *data;
  uint8_t *temp_data;
};

void HandleSerialManagerTask(void *pvParameters);
#endif