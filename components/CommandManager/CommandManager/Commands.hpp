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
  virtual ~Command() = default;
  virtual CommandResult execute(std::string &jsonPayload) = 0;
};

class PingCommand : public Command
{
public:
  CommandResult execute(std::string &jsonPayload) override;
};

class setWiFiCommand : public Command
{
  ProjectConfig &projectConfig;

public:
  setWiFiCommand(ProjectConfig &projectConfig) : projectConfig(projectConfig) {};
  CommandResult execute(std::string &jsonPayload) override;
  std::optional<WifiPayload> parsePayload(std::string &jsonPayload);
};

class deleteWifiCommand : public Command
{
  ProjectConfig &projectConfig;

public:
  deleteWifiCommand(ProjectConfig &projectConfig) : projectConfig(projectConfig) {};
  CommandResult execute(std::string &jsonPayload) override;
  std::optional<deleteNetworkPayload> parsePayload(std::string &jsonPayload);
};

class updateWifiCommand : public Command
{
  ProjectConfig &projectConfig;

public:
  updateWifiCommand(ProjectConfig &projectConfig) : projectConfig(projectConfig) {};
  CommandResult execute(std::string &jsonPayload) override;
  std::optional<UpdateWifiPayload> parsePayload(std::string &jsonPayload);
};

class updateAPWiFiCommand : public Command
{
  ProjectConfig &projectConfig;

public:
  updateAPWiFiCommand(ProjectConfig &projectConfig) : projectConfig(projectConfig) {};
  CommandResult execute(std::string &jsonPayload) override;
  std::optional<UpdateAPWiFiPayload> parsePayload(std::string &jsonPayload);
};

class setMDNSCommand : public Command
{
  ProjectConfig &projectConfig;

public:
  setMDNSCommand(ProjectConfig &projectConfig) : projectConfig(projectConfig) {};
  CommandResult execute(std::string &jsonPayload) override;
  std::optional<MDNSPayload> parsePayload(std::string &jsonPayload);
};

class updateCameraCommand : public Command
{
  ProjectConfig &projectConfig;

public:
  updateCameraCommand(ProjectConfig &projectConfig) : projectConfig(projectConfig) {};
  CommandResult execute(std::string &jsonPayload) override;
  std::optional<UpdateCameraConfigPayload> parsePayload(std::string &jsonPayload);
};

class saveConfigCommand : public Command
{
  ProjectConfig &projectConfig;
  saveConfigCommand(ProjectConfig &projectConfig) : projectConfig(projectConfig) {};
  CommandResult execute(std::string &jsonPayload) override;
};

// mostlikely missing commands
// reset config
// restart device

#endif