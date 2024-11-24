#ifndef COMMANDS_HPP
#define COMMANDS_HPP
#include <ProjectConfig.hpp>
#include <memory>
#include <string>
#include <optional>
#include <unordered_map>
#include <cJSON.h>
#include "CommandResult.hpp"
#include "CommandSchema.hpp"

class Command
{
public:
  Command() = default;
  virtual ~Command() = default;
  virtual CommandResult execute(std::string_view jsonPayload) = 0;
};

class PingCommand : public Command
{
public:
  PingCommand() = default;
  CommandResult execute(std::string_view jsonPayload) override;
};

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

class setMDNSCommand : public Command
{
  std::shared_ptr<ProjectConfig> projectConfig;

public:
  setMDNSCommand(std::shared_ptr<ProjectConfig> projectConfig) : projectConfig(projectConfig) {};
  CommandResult execute(std::string_view jsonPayload) override;
  std::optional<MDNSPayload> parsePayload(std::string_view jsonPayload);
};

class updateCameraCommand : public Command
{
  std::shared_ptr<ProjectConfig> projectConfig;

public:
  updateCameraCommand(std::shared_ptr<ProjectConfig> projectConfig) : projectConfig(projectConfig) {};
  CommandResult execute(std::string_view jsonPayload) override;
  std::optional<UpdateCameraConfigPayload> parsePayload(std::string_view jsonPayload);
};

class saveConfigCommand : public Command
{
public:
  std::shared_ptr<ProjectConfig> projectConfig;
  saveConfigCommand(std::shared_ptr<ProjectConfig> projectConfig) : projectConfig(projectConfig) {};
  CommandResult execute(std::string_view jsonPayload) override;
};

// mostlikely missing commands
// reset config
// restart device

#endif