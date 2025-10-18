#include "ProjectConfig.hpp"

static auto CONFIGURATION_TAG = "[CONFIGURATION]";

int getNetworkCount(Preferences *pref)
{
  return pref->getInt("networkcount", 0);
}

void saveNetworkCount(Preferences *pref, const int count)
{
  pref->putInt("networkcount", count);
}

ProjectConfig::ProjectConfig(Preferences *pref) : pref(pref),
                                                  _already_loaded(false),
                                                  config(DeviceConfig_t(pref),
                                                         DeviceMode_t(pref),
                                                         CameraConfig_t(pref),
                                                         std::vector<WiFiConfig_t>{},
                                                         AP_WiFiConfig_t(pref),
                                                         MDNSConfig_t(pref),
                                                         WiFiTxPower_t(pref)) {}

ProjectConfig::~ProjectConfig() = default;

void ProjectConfig::save() const
{
  ESP_LOGD(CONFIGURATION_TAG, "Saving project config");
  this->config.device.save();
  this->config.device_mode.save();
  this->config.camera.save();
  this->config.mdns.save();
  this->config.txpower.save();
  this->config.ap_network.save();

  auto const networks_count = static_cast<int>(this->config.networks.size());
  for (int i = 0; i < networks_count; i++)
  {
    this->config.networks[i].save();
  }

  saveNetworkCount(this->pref, networks_count);
  this->pref->end(); // we call end() here to close the connection to the NVS partition, we
                     // only do this because we call ESP.restart() next.

  // TODO add the restart task
  // https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_idf.html
  // OpenIrisTasks::ScheduleRestart(2000);
}

void ProjectConfig::load()
{
  ESP_LOGD(CONFIGURATION_TAG, "Loading project config");
  if (this->_already_loaded)
  {
    ESP_LOGW(CONFIGURATION_TAG, "Project config already loaded");
    return;
  }

  const bool success = this->pref->begin("openiris");

  ESP_LOGI(CONFIGURATION_TAG, "Config name: openiris");
  ESP_LOGI(CONFIGURATION_TAG, "Config loaded: %s", success ? "true" : "false");

  this->config.device.load();
  this->config.device_mode.load();
  this->config.camera.load();
  this->config.mdns.load();
  this->config.txpower.load();
  this->config.ap_network.load();

  const auto networks_count = getNetworkCount(this->pref);
  ESP_LOGD(CONFIGURATION_TAG, "Loading networks: %d", networks_count);
  for (int i = 0; i < getNetworkCount(this->pref); i++)
  {
    auto networkConfig = WiFiConfig_t(this->pref);
    networkConfig.load(i);
    this->config.networks.push_back(networkConfig);
  }

  this->_already_loaded = true;
}

bool ProjectConfig::reset()
{
  ESP_LOGW(CONFIGURATION_TAG, "Resetting project config");
  return this->pref->clear();
}

//**********************************************************************************************************************
//*
//!                                                DeviceConfig
//*
//**********************************************************************************************************************
void ProjectConfig::setOTAConfig(const std::string &OTALogin,
                                 const std::string &OTAPassword,
                                 const int OTAPort)
{
  ESP_LOGD(CONFIGURATION_TAG, "Updating device config");
  this->config.device.OTALogin.assign(OTALogin);
  this->config.device.OTAPassword.assign(OTAPassword);
  this->config.device.OTAPort = OTAPort;
  this->config.device.save();
}

void ProjectConfig::setLEDDUtyCycleConfig(int led_external_pwm_duty_cycle)
{
  this->config.device.led_external_pwm_duty_cycle = led_external_pwm_duty_cycle;
  ESP_LOGI(CONFIGURATION_TAG, "Setting duty cycle to %d", led_external_pwm_duty_cycle);
  this->config.device.save();
}

void ProjectConfig::setMDNSConfig(const std::string &hostname)
{
  ESP_LOGD(CONFIGURATION_TAG, "Updating MDNS config");
  this->config.mdns.hostname.assign(hostname);
  this->config.mdns.save();
}

void ProjectConfig::setCameraConfig(const uint8_t vflip,
                                    const uint8_t framesize,
                                    const uint8_t href,
                                    const uint8_t quality,
                                    const uint8_t brightness)
{
  ESP_LOGD(CONFIGURATION_TAG, "Updating camera config");
  this->config.camera.vflip = vflip;
  this->config.camera.href = href;
  this->config.camera.framesize = framesize;
  this->config.camera.quality = quality;
  this->config.camera.brightness = brightness;
  this->config.camera.save();

  ESP_LOGD(CONFIGURATION_TAG, "Updating Camera config");
}

void ProjectConfig::setWifiConfig(const std::string &networkName,
                                  const std::string &ssid,
                                  const std::string &password,
                                  uint8_t channel,
                                  uint8_t power)
{
  const auto size = this->config.networks.size();

  const auto it = std::ranges::find_if(this->config.networks,
                                       [&](const WiFiConfig_t &network)
                                       { return network.name == networkName; });

  if (it != this->config.networks.end())
  {

    ESP_LOGI(CONFIGURATION_TAG, "Found network %s, updating it ...",
             it->name.c_str());

    it->name = networkName;
    it->ssid = ssid;
    it->password = password;
    it->channel = channel;
    it->power = power;
    // Save the updated network immediately
    it->save();
    return;
  }

  if (size == 0)
  {
    ESP_LOGI(CONFIGURATION_TAG, "No networks, We're adding a new network");
    this->config.networks.emplace_back(this->pref, static_cast<uint8_t>(0), networkName, ssid, password, channel,
                                       power);
    // Save the new network immediately
    this->config.networks.back().save();
    saveNetworkCount(this->pref, 1);
    return;
  }

  // we're allowing to store up to three additional networks
  if (size < 3)
  {
    ESP_LOGI(CONFIGURATION_TAG, "We're adding a new network");
    // we don't have that network yet, we can add it as we still have some
    // space we're using emplace_back as push_back will create a copy of it,
    // we want to avoid that
    uint8_t last_index = getNetworkCount(this->pref);
    this->config.networks.emplace_back(this->pref, last_index, networkName, ssid, password, channel,
                                       power);
    // Save the new network immediately
    this->config.networks.back().save();
    saveNetworkCount(this->pref, static_cast<int>(this->config.networks.size()));
  }
  else
  {
    ESP_LOGE(CONFIGURATION_TAG, "No more space for additional networks");
  }
}

void ProjectConfig::deleteWifiConfig(const std::string &networkName)
{
  if (const auto size = this->config.networks.size(); size == 0)
  {
    ESP_LOGI(CONFIGURATION_TAG, "No networks, nothing to delete");
  }

  const auto it = std::ranges::find_if(this->config.networks,
                                       [&](const WiFiConfig_t &network)
                                       { return network.name == networkName; });

  if (it != this->config.networks.end())
  {
    ESP_LOGI(CONFIGURATION_TAG, "Found network %s", it->name.c_str());
    this->config.networks.erase(it);
    ESP_LOGI(CONFIGURATION_TAG, "Deleted network %s", networkName.c_str());
  }
}

void ProjectConfig::setWiFiTxPower(uint8_t power)
{
  this->config.txpower.power = power;
  this->config.txpower.save();
  ESP_LOGD(CONFIGURATION_TAG, "Updating wifi tx power");
}

void ProjectConfig::setAPWifiConfig(const std::string &ssid,
                                    const std::string &password,
                                    const uint8_t channel)
{
  this->config.ap_network.ssid.assign(ssid);
  this->config.ap_network.password.assign(password);
  this->config.ap_network.channel = channel;
  this->config.ap_network.save();
  ESP_LOGD(CONFIGURATION_TAG, "Updating access point config");
}

void ProjectConfig::setDeviceMode(const StreamingMode deviceMode)
{
  this->config.device_mode.mode = deviceMode;
  this->config.device_mode.save(); // Save immediately
}

//**********************************************************************************************************************
//*
//!                                                Get Methods
//*
//**********************************************************************************************************************

DeviceConfig_t &ProjectConfig::getDeviceConfig()
{
  return this->config.device;
}
CameraConfig_t &ProjectConfig::getCameraConfig()
{
  return this->config.camera;
}
std::vector<WiFiConfig_t> &ProjectConfig::getWifiConfigs()
{
  return this->config.networks;
}
AP_WiFiConfig_t &ProjectConfig::getAPWifiConfig()
{
  return this->config.ap_network;
}
MDNSConfig_t &ProjectConfig::getMDNSConfig()
{
  return this->config.mdns;
}
WiFiTxPower_t &ProjectConfig::getWiFiTxPowerConfig()
{
  return this->config.txpower;
}
TrackerConfig_t &ProjectConfig::getTrackerConfig()
{
  return this->config;
}

DeviceMode_t &ProjectConfig::getDeviceModeConfig()
{
  return this->config.device_mode;
}

StreamingMode ProjectConfig::getDeviceMode()
{
  return this->config.device_mode.mode;
}