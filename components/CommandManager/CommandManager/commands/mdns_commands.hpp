#include <ProjectConfig.hpp>
#include <memory>
#include <string>
#include <optional>
#include <cJSON.h>
#include "CommandResult.hpp"
#include "CommandSchema.hpp"
#include "DependencyRegistry.hpp"

std::optional<MDNSPayload> parseMDNSCommandPayload(std::string_view jsonPayload);
CommandResult setMDNSCommand(std::shared_ptr<DependencyRegistry> registry, std::string_view jsonPayload);
CommandResult getMDNSNameCommand(std::shared_ptr<DependencyRegistry> registry);