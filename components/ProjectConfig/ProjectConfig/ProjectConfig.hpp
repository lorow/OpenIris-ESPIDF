#pragma once
#ifndef _PROJECT_CONFIG_HPP_
#define _PROJECT_CONFIG_HPP_
#include "esp_log.h"
#include <algorithm>
#include <vector>
#include <string>
#include <helpers.hpp>
#include "Models.hpp"
#include <Preferences.hpp>

static const char *CONFIGURATION_TAG = "[CONFIGURATION]";

int getNetworkCount(Preferences *pref);

void saveNetworkCount(Preferences *pref, int count);

class ProjectConfig
{
public:
  ProjectConfig(Preferences *pref);
  virtual ~ProjectConfig();

  void load();
  void save();

  void wifiConfigSave();
  void cameraConfigSave();
  void deviceConfigSave();
  void mdnsConfigSave();
  void wifiTxPowerConfigSave();
  bool reset();

  DeviceConfig_t &getDeviceConfig();
  CameraConfig_t &getCameraConfig();
  std::vector<WiFiConfig_t> &getWifiConfigs();
  AP_WiFiConfig_t &getAPWifiConfig();
  MDNSConfig_t &getMDNSConfig();
  WiFiTxPower_t &getWiFiTxPowerConfig();
  TrackerConfig_t &getTrackerConfig();

  void setDeviceConfig(const std::string &OTALogin,
                       const std::string &OTAPassword,
                       int OTAPort);
  void setMDNSConfig(const std::string &hostname);
  void setCameraConfig(uint8_t vflip,
                       uint8_t framesize,
                       uint8_t href,
                       uint8_t quality,
                       uint8_t brightness);
  void setWifiConfig(const std::string &networkName,
                     const std::string &ssid,
                     const std::string &password,
                     uint8_t channel,
                     uint8_t power);

  void deleteWifiConfig(const std::string &networkName);

  void setAPWifiConfig(const std::string &ssid,
                       const std::string &password,
                       uint8_t channel);
  void setWiFiTxPower(uint8_t power);

private:
  Preferences *pref;
  bool _already_loaded;
  TrackerConfig_t config;
};

#endif