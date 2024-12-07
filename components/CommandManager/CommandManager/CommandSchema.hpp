#ifndef COMMAND_SCHEMA_HPP
#define COMMAND_SCHEMA_HPP

struct BasePayload
{
};

struct WifiPayload : BasePayload
{
  std::string networkName;
  std::string ssid;
  std::string password;
  uint8_t channel;
  uint8_t power;
};

struct UpdateWifiPayload : BasePayload
{
  std::string networkName;
  std::optional<std::string> ssid;
  std::optional<std::string> password;
  std::optional<uint8_t> channel;
  std::optional<uint8_t> power;
};

struct deleteNetworkPayload : BasePayload
{
  std::string networkName;
};

// implement
struct UpdateAPWiFiPayload : BasePayload
{
  std::string ssid;
  std::string password;
  uint8_t channel;
};

struct MDNSPayload : public BasePayload
{
  std::string hostname;
};

struct UpdateCameraConfigPayload : BasePayload
{
  std::optional<uint8_t> vflip;
  std::optional<uint8_t> href;
  std::optional<uint8_t> framesize;
  std::optional<uint8_t> quality;
  std::optional<uint8_t> brightness;
  // TODO add more options here
};

struct ResetConfigPayload : BasePayload
{
  std::string section;
};

struct RestartCameraPayload : BasePayload
{
  bool mode;
};
#endif