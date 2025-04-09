#include <ProjectConfig.hpp>
#include <memory>
#include <string>
#include <optional>
#include <cJSON.h>
#include "CommandResult.hpp"
#include "CommandSchema.hpp"
#include "DependencyRegistry.hpp"

std::optional<WifiPayload> parseSetWiFiCommandPayload(std::string_view jsonPayload);
CommandResult setWiFiCommand(std::shared_ptr<DependencyRegistry> registry, std::string_view jsonPayload);

std::optional<deleteNetworkPayload> parseDeleteWifiCommandPayload(std::string_view jsonPayload);
CommandResult deleteWiFiCommand(std::shared_ptr<DependencyRegistry> registry, std::string_view jsonPayload);

std::optional<UpdateWifiPayload> parseUpdateWifiCommandPayload(std::string_view jsonPayload);
CommandResult updateWiFiCommand(std::shared_ptr<DependencyRegistry> registry, std::string_view jsonPayload);

std::optional<UpdateAPWiFiPayload> parseUpdateAPWiFiCommandPayload(std::string_view jsonPayload);
CommandResult updateAPWiFiCommand(std::shared_ptr<DependencyRegistry> registry, std::string_view jsonPayload);
