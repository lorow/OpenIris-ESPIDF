#include "BaseCommand.hpp"

class setWiFiCommand : public Command
{
  std::shared_ptr<ProjectConfig> projectConfig;

public:
  setWiFiCommand(std::shared_ptr<ProjectConfig> projectConfig) : projectConfig(projectConfig) {};
  CommandResult execute(std::string_view jsonPayload) override;
  std::optional<WifiPayload> parsePayload(std::string_view jsonPayload);
};

class deleteWifiCommand : public Command
{
  std::shared_ptr<ProjectConfig> projectConfig;

public:
  deleteWifiCommand(std::shared_ptr<ProjectConfig> projectConfig) : projectConfig(projectConfig) {};
  CommandResult execute(std::string_view jsonPayload) override;
  std::optional<deleteNetworkPayload> parsePayload(std::string_view jsonPayload);
};

class updateWifiCommand : public Command
{
  std::shared_ptr<ProjectConfig> projectConfig;

public:
  updateWifiCommand(std::shared_ptr<ProjectConfig> projectConfig) : projectConfig(projectConfig) {};
  CommandResult execute(std::string_view jsonPayload) override;
  std::optional<UpdateWifiPayload> parsePayload(std::string_view jsonPayload);
};

class updateAPWiFiCommand : public Command
{
  std::shared_ptr<ProjectConfig> projectConfig;

public:
  updateAPWiFiCommand(std::shared_ptr<ProjectConfig> projectConfig) : projectConfig(projectConfig) {};
  CommandResult execute(std::string_view jsonPayload) override;
  std::optional<UpdateAPWiFiPayload> parsePayload(std::string_view jsonPayload);
};
