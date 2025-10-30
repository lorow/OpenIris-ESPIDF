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
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_vfs_dev.h"
#include "esp_mac.h"

#ifndef BUF_SIZE
#define BUF_SIZE (1024)
#endif

extern "C" void tud_cdc_rx_cb(uint8_t itf);
extern "C" void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts);

extern QueueHandle_t cdcMessageQueue;

struct cdc_command_packet_t
{
  uint8_t len;
  uint8_t data[64];
};

class SerialManager
{
public:
  explicit SerialManager(std::shared_ptr<CommandManager> commandManager, esp_timer_handle_t *timerHandle);
  void setup();
  void try_receive();
  void notify_startup_command_received();
  void shutdown();

private:
  std::shared_ptr<CommandManager> commandManager;
  esp_timer_handle_t *timerHandle;
  uint8_t *data;
  uint8_t *temp_data;
};

void HandleSerialManagerTask(void *pvParameters);
void HandleCDCSerialManagerTask(void *pvParameters);
#endif