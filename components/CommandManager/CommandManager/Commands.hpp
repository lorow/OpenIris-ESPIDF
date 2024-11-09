#ifndef COMMANDS_HPP
#define COMMANDS_HPP
#include <ProjectConfig.hpp>
#include <memory>
#include <string>
#include <optional>
#include <unordered_map>
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
  WifiPayload parsePayload(std::string &jsonPayload);
};

#endif