#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <ProjectConfig.hpp>
#include <memory>
#include <string>
#include <optional>
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

#endif