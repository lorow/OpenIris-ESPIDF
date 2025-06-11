#pragma once
#ifndef PROJECT_CONFIG_MODELS_HPP
#define PROJECT_CONFIG_MODELS_HPP

#include <string>
#include <utility>
#include <vector>
#include <helpers.hpp>
#include "sdkconfig.h"
#include <Preferences.hpp>
#include "esp_log.h"

struct BaseConfigModel
{
  BaseConfigModel(Preferences *pref) : pref(pref) {}

  void load();
  void save();
  std::string toRepresentation();

  Preferences *pref;
};

enum class StreamingMode {
  AUTO,
  UVC,
  WIFI,
};

struct DeviceMode_t : BaseConfigModel {
  StreamingMode mode;
  explicit DeviceMode_t(  Preferences *pref) : BaseConfigModel(pref), mode(StreamingMode::AUTO){}

  void load() {
    int stored_mode = this->pref->getInt("mode", 0);
    this->mode = static_cast<StreamingMode>(stored_mode);
    ESP_LOGI("DeviceMode", "Loaded device mode: %d", stored_mode);
  }

  void save() const {
    this->pref->putInt("mode", static_cast<int>(this->mode));
    ESP_LOGI("DeviceMode", "Saved device mode: %d", static_cast<int>(this->mode));
  }
};


struct DeviceConfig_t : BaseConfigModel
{
  DeviceConfig_t(Preferences *pref) : BaseConfigModel(pref) {}

  std::string OTALogin;
  std::string OTAPassword;
  int OTAPort;

  void load()
  {
    this->OTALogin = this->pref->getString("OTALogin", "openiris");
    this->OTAPassword = this->pref->getString("OTAPassword", "openiris");
    this->OTAPort = this->pref->getInt("OTAPort", 3232);
  };

  void save() const {
    this->pref->putString("OTALogin", this->OTALogin.c_str());
    this->pref->putString("OTAPassword", this->OTAPassword.c_str());
    this->pref->putInt("OTAPort", this->OTAPort);
  };

  std::string toRepresentation() const
  {
    return Helpers::format_string(
        "\"device_config\": {\"OTALogin\": \"%s\", \"OTAPassword\": \"%s\", "
        "\"OTAPort\": %u}",
        this->OTALogin.c_str(), this->OTAPassword.c_str(), this->OTAPort);
  };
};

struct MDNSConfig_t : BaseConfigModel
{
  MDNSConfig_t(Preferences *pref) : BaseConfigModel(pref) {}

  std::string hostname;

  void load()
  {
    // by default, this will be openiris
    // but we can override it at compile time
    std::string default_hostname = CONFIG_MDNS_HOSTNAME;

    if (default_hostname.empty())
    {
      default_hostname = "openiristracker";
    }

    this->hostname = this->pref->getString("hostname", default_hostname);
  };

  void save() const {
    this->pref->putString("hostname", this->hostname.c_str());
  };

  std::string toRepresentation()
  {
    return Helpers::format_string(
        "\"mdns_config\": {\"hostname\": \"%s\"}",
        this->hostname.c_str());
  };
};

struct CameraConfig_t : BaseConfigModel
{
  CameraConfig_t(Preferences *pref) : BaseConfigModel(pref) {}

  uint8_t vflip;
  uint8_t href;
  uint8_t framesize;
  uint8_t quality;
  uint8_t brightness;

  void load()
  {
    this->vflip = this->pref->getInt("vflip", 0);
    this->href = this->pref->getInt("href", 0);
    this->framesize = this->pref->getInt("framesize", 4);
    this->quality = this->pref->getInt("quality", 7);
    this->brightness = this->pref->getInt("brightness", 2);
  };

  void save() const {
    this->pref->putInt("vflip", this->vflip);
    this->pref->putInt("href", this->href);
    this->pref->putInt("framesize", this->framesize);
    this->pref->putInt("quality", this->quality);
    this->pref->putInt("brightness", this->brightness);
  };

  std::string toRepresentation()
  {
    return Helpers::format_string(
        "\"camera_config\": {\"vflip\": %d,\"framesize\": %d,\"href\": "
        "%d,\"quality\": %d,\"brightness\": %d}",
        this->vflip, this->framesize, this->href, this->quality,
        this->brightness);
  };
};

// with wifi, we have to work a bit differently
// we can have multiple networks saved
// so, we not only need to keep track of them, we also have to
// save them under an indexed name and load them as such.
struct WiFiConfig_t : BaseConfigModel
{
  // default constructor used for loading
  WiFiConfig_t(Preferences *pref) : BaseConfigModel(pref) {}

  WiFiConfig_t(
      Preferences *pref,
      const uint8_t index,
      std::string name,
      std::string ssid,
      std::string password,
      const uint8_t channel,
      const uint8_t power)
      : BaseConfigModel(pref),
        index(index),
        name(std::move(name)),
        ssid(std::move(ssid)),
        password(std::move(password)),
        channel(channel),
        power(power) {}

  uint8_t index;
  std::string name;
  std::string ssid;
  std::string password;
  uint8_t channel;
  uint8_t power;

  void load(const uint8_t index)
  {
    this->index = index;
    char buffer[2];

    auto const iter_str = std::string(Helpers::itoa(index, buffer, 10));
    this->name = this->pref->getString(("name" + iter_str).c_str(), "");
    this->ssid = this->pref->getString(("ssid" + iter_str).c_str(), "");
    this->password = this->pref->getString(("password" + iter_str).c_str(), "");
    this->channel = this->pref->getUInt(("channel" + iter_str).c_str());
    this->power = this->pref->getUInt(("power" + iter_str).c_str());
    
    ESP_LOGI("WiFiConfig", "Loaded network %d: name=%s, ssid=%s, channel=%d", 
             index, this->name.c_str(), this->ssid.c_str(), this->channel);
  };

  void save() const {
    char buffer[2];
    auto const iter_str = std::string(Helpers::itoa(this->index, buffer, 10));

    this->pref->putString(("name" + iter_str).c_str(), this->name.c_str());
    this->pref->putString(("ssid" + iter_str).c_str(), this->ssid.c_str());
    this->pref->putString(("password" + iter_str).c_str(), this->password.c_str());
    this->pref->putUInt(("channel" + iter_str).c_str(), this->channel);
    this->pref->putUInt(("power" + iter_str).c_str(), this->power);
    
    ESP_LOGI("WiFiConfig", "Saved network %d: name=%s, ssid=%s, channel=%d", 
             this->index, this->name.c_str(), this->ssid.c_str(), this->channel);
  };

  std::string toRepresentation()
  {
    return Helpers::format_string(
        "{\"name\": \"%s\", \"ssid\": \"%s\", \"password\": \"%s\", \"channel\": %u, \"power\": %u}",
        this->name.c_str(), this->ssid.c_str(), this->password.c_str(),
        this->channel, this->power);
  };
};

struct AP_WiFiConfig_t : BaseConfigModel
{
  AP_WiFiConfig_t(Preferences *pref) : BaseConfigModel(pref) {}

  std::string ssid;
  std::string password;
  uint8_t channel;

  void load()
  {
    this->ssid = this->pref->getString("apSSID", CONFIG_AP_WIFI_SSID);
    this->password = this->pref->getString("apPassword", CONFIG_AP_WIFI_PASSWORD);
  };

  void save() const {
    this->pref->putString("apSSID", this->ssid.c_str());
    this->pref->putString("apPass", this->password.c_str());
    this->pref->putUInt("apChannel", this->channel);
  };

  std::string toRepresentation()
  {
    return Helpers::format_string(
        "\"ap_wifi_config\": {\"ssid\": \"%s\", \"password\": \"%s\", "
        "\"channel\": %u}",
        this->ssid.c_str(), this->password.c_str(), this->channel);
  };
};

struct WiFiTxPower_t : BaseConfigModel
{
  WiFiTxPower_t(Preferences *pref) : BaseConfigModel(pref) {}

  uint8_t power;

  void load()
  {
    this->power = this->pref->getUInt("txpower", 52);
  };

  void save() const {
    this->pref->putUInt("txpower", this->power);
  };

  std::string toRepresentation()
  {
    return Helpers::format_string("\"wifi_tx_power\": {\"power\": %u}", this->power);
  };
};

class TrackerConfig_t
{
public:
  DeviceConfig_t device;
  DeviceMode_t device_mode;
  CameraConfig_t camera;
  std::vector<WiFiConfig_t> networks;
  AP_WiFiConfig_t ap_network;
  MDNSConfig_t mdns;
  WiFiTxPower_t txpower;

  TrackerConfig_t(
      DeviceConfig_t device,
      DeviceMode_t device_mode,
      CameraConfig_t camera,
      std::vector<WiFiConfig_t> networks,
      AP_WiFiConfig_t ap_network,
      MDNSConfig_t mdns,
      WiFiTxPower_t txpower) : device(std::move(device)),
                               device_mode(std::move(device_mode)),
                               camera(std::move(camera)),
                               networks(std::move(networks)),
                               ap_network(std::move(ap_network)),
                               mdns(std::move(mdns)),
                               txpower(std::move(txpower)) {}

  std::string toRepresentation()
  {
    std::string WifiConfigRepresentation;

    // we need a valid json representation, so we can't have a dangling comma at the end
    if (!this->networks.empty())
    {
      if (this->networks.size() > 1)
      {
        for (auto i = 0; i < this->networks.size() - 1; i++)
        {
          printf("we're at %d while networks size is %d ", i, this->networks.size() - 2);
          WifiConfigRepresentation += Helpers::format_string("%s, ", this->networks[i].toRepresentation().c_str());
        }
      }

      WifiConfigRepresentation += Helpers::format_string("%s", this->networks[networks.size() - 1].toRepresentation().c_str());
      printf(WifiConfigRepresentation.c_str());
      printf("\n");
    }

    return Helpers::format_string(
        "{%s, %s, %s, \"networks\": [%s], %s, %s}",
        this->device.toRepresentation().c_str(),
        this->mdns.toRepresentation().c_str(),
        this->camera.toRepresentation().c_str(),
        WifiConfigRepresentation.c_str(),
        this->ap_network.toRepresentation().c_str(),
        this->txpower.toRepresentation().c_str());
  }
};

#endif