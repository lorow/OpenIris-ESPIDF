#include <ProjectConfig.hpp>
#include <memory>
#include <string>
#include <optional>
#include <cJSON.h>
#include "CommandResult.hpp"
#include "CommandSchema.hpp"
#include "DependencyRegistry.hpp"

CommandResult saveConfigCommand(std::shared_ptr<DependencyRegistry> registry);
CommandResult getConfigCommand(std::shared_ptr<DependencyRegistry> registry);

std::optional<ResetConfigPayload> parseresetConfigCommandPayload(std::string_view jsonPayload);
CommandResult resetConfigCommand(std::shared_ptr<DependencyRegistry> registry, std::string_view jsonPayload);