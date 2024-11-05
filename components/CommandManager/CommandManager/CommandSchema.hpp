class BasePayload
{
};

class WifiPayload : public BasePayload
{
  std::string networkName;
  std::string ssid;
  std::string password;
  uint8_t channel;
  uint8_t power;
};

class UpdateWifiPayload : public BasePayload
{
  std::optional<std::string> networkName;
  std::optional<std::string> ssid;
  std::optional<std::string> password;
  std::optional<uint8_t> channel;
  std::optional<uint8_t> power;
};

class deleteNetworkPayload : public BasePayload
{
  std::optional<std::string> networkName;
};