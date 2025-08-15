#pragma once
#ifndef SERIALMANAGER_HPP
#define SERIALMANAGER_HPP

#include <stdio.h>
#include <string>
#include <memory>
#include <CommandManager.hpp>
#include <ProjectConfig.hpp>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/usb_serial_jtag.h"
#include "esp_vfs_usb_serial_jtag.h"
#include "esp_vfs_dev.h"
#include "esp_mac.h"

class SerialManager
{
public:
  explicit SerialManager(std::shared_ptr<CommandManager> commandManager, esp_timer_handle_t *timerHandle, std::shared_ptr<ProjectConfig> deviceConfig);
  void setup();
  void try_receive();
  void send_heartbeat();
  bool should_send_heartbeat();
  void notify_startup_command_received();

private:
  std::shared_ptr<CommandManager> commandManager;
  esp_timer_handle_t *timerHandle;
  std::shared_ptr<ProjectConfig> deviceConfig;
  uint8_t *data;
  uint8_t *temp_data;
};

void HandleSerialManagerTask(void *pvParameters);
#endif