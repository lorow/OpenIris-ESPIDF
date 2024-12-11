#pragma once
#ifndef MDNSMANAGER_HPP
#define MDNSMANAGER_HPP
#include <string>
#include <ProjectConfig.hpp>
#include <StateManager.hpp>
#include "esp_log.h"
#include "mdns.h"

class MDNSManager
{
private:
  std::shared_ptr<ProjectConfig> projectConfig;

public:
  MDNSManager(std::shared_ptr<ProjectConfig> projectConfig);
  esp_err_t start();
};

#endif // MDNSMANAGER_HPP
