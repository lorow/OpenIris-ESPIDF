#pragma once
#ifndef _PROJECT_CONFIG_MODELS_HPP_
#define _PROJECT_CONFIG_MODELS_HPP_

#include <string>
#include <vector>
#include <helpers.hpp>
#include "sdkconfig.h"
#include <Preferences.hpp>

struct BaseConfigModel
{
  BaseConfigModel(Preferences *pref) : pref(pref) {}

  void load();
  void save();
  std::string toRepresentation();

  Preferences *pref;
};

struct DeviceConfig_t : BaseConfigModel
{
  DeviceConfig_t(Preferences *pref) : BaseConfigModel(pref) {}

  std::string OTALogin;
  std::string OTAPassword;
  int OTAPort;

  void load()
  {
    this->OTALogin = this->pref->getString("OTALogin", "openiris").c_str();
    this->OTAPassword = this->pref->getString("OTAPassword", "openiris").c_str();
    this->OTAPort = this->pref->getInt("OTAPort", 3232);
  };

  void save()
  {
    this->pref->putString("OTALogin", this->OTALogin.c_str());
    this->pref->putString("OTAPassword", this->OTAPassword.c_str());
    this->pref->putInt("OTAPort", this->OTAPort);
  };

  std::string toRepresentation()
  {
    return Helpers::format_string(
        "\"device_config\": {\"OTALogin\": \"%s\", \"OTAPassword\": \"%s\", "
        "\"OTAPort\": %u}",
        this->OTALogin.c_str(), this->OTAPassword.c_str(), this->OTAPort);
  };
};

struct MDNSConfig_t : public BaseConfigModel
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

    this->hostname = this->pref->getString("hostname", default_hostname).c_str();
  };

  void save()
  {
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

  void save()
  {
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
  // default construcotr used for loading
  WiFiConfig_t(Preferences *pref) : BaseConfigModel(pref) {}

  WiFiConfig_t(
      Preferences *pref,
      uint8_t index,
      const std::string &name,
      const std::string &ssid,
      const std::string &password,
      uint8_t channel,
      uint8_t power)
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

  void load(uint8_t index)
  {
    this->index = index;
    char buffer[2];

    std::string iter_str = Helpers::itoa(index, buffer, 10);
    this->name = this->pref->getString(("name" + iter_str).c_str(), "").c_str();
    this->ssid = this->pref->getString(("ssid" + iter_str).c_str(), "").c_str();
    this->password = this->pref->getString(("password" + iter_str).c_str(), "").c_str();
    this->channel = this->pref->getUInt(("channel" + iter_str).c_str());
    this->power = this->pref->getUInt(("power" + iter_str).c_str());
  };

  void save()
  {
    char buffer[2];
    std::string iter_str = Helpers::itoa(this->index, buffer, 10);

    this->pref->putString(("name" + iter_str).c_str(), this->name.c_str());
    this->pref->putString(("ssid" + iter_str).c_str(), this->ssid.c_str());
    this->pref->putString(("password" + iter_str).c_str(), this->password.c_str());
    this->pref->putUInt(("channel" + iter_str).c_str(), this->channel);
    this->pref->putUInt(("power" + iter_str).c_str(), this->power);
  };

  std::string toRepresentation()
  {
    return Helpers::format_string(
        "{\"name\": \"%s\", \"ssid\": \"%s\", \"password\": \"%s\", "
        "\"channel\": "
        "%u, \"power\": %u}",
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
    this->ssid = this->pref->getString("apSSID", CONFIG_AP_WIFI_SSID).c_str();
    this->password = this->pref->getString("apPassword", CONFIG_AP_WIFI_PASSWORD).c_str();
  };

  void save()
  {
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

  void save()
  {
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
  CameraConfig_t camera;
  std::vector<WiFiConfig_t> networks;
  AP_WiFiConfig_t ap_network;
  MDNSConfig_t mdns;
  WiFiTxPower_t txpower;

  TrackerConfig_t(
      DeviceConfig_t device,
      CameraConfig_t camera,
      std::vector<WiFiConfig_t> networks,
      AP_WiFiConfig_t ap_network,
      MDNSConfig_t mdns,
      WiFiTxPower_t txpower) : device(device),
                               camera(camera),
                               networks(networks),
                               ap_network(ap_network),
                               mdns(mdns),
                               txpower(txpower) {}

  std::string toRepresentation()
  {
    std::string WifiConfigRepresentation = "";

    for (auto &network : this->networks)
    {
      WifiConfigRepresentation += Helpers::format_string(", %s", network.toRepresentation());
    }

    return Helpers::format_string(
        "%s, %s, %s, %s, %s, %s",
        this->device.toRepresentation().c_str(),
        this->mdns.toRepresentation().c_str(),
        this->camera.toRepresentation().c_str(),
        WifiConfigRepresentation.c_str(),
        this->mdns.toRepresentation().c_str(),
        this->txpower.toRepresentation().c_str());
  }
};

#endif