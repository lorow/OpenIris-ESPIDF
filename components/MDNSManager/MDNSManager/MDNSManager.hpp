#pragma once
#ifndef MDNSMANAGER_HPP
#define MDNSMANAGER_HPP
#include <string>
#include <ProjectConfig.hpp>
#include <StateManager.hpp>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "mdns.h"

class MDNSManager
{
private:
  std::shared_ptr<ProjectConfig> projectConfig;
  QueueHandle_t eventQueue;

public:
  MDNSManager(std::shared_ptr<ProjectConfig> projectConfig, QueueHandle_t eventQueue);
  esp_err_t start();
};

#endif // MDNSMANAGER_HPP
