#pragma once
#ifndef MDNSMANAGER_HPP
#define MDNSMANAGER_HPP
#include <string>
#include <ProjectConfig.hpp>
#include <StateManager.hpp>
#include "esp_log.h"
#include "mdns.h"

// TODO add observer pattern here
class MDNSManager
{
private:
  ProjectConfig &projectConfig;

public:
  MDNSManager(ProjectConfig &projectConfig);
  esp_err_t start();
};

#endif // MDNSMANAGER_HPP
