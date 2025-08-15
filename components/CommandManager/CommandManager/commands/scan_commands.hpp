#ifndef SCAN_COMMANDS_HPP
#define SCAN_COMMANDS_HPP

#include "CommandResult.hpp"
#include "DependencyRegistry.hpp"
#include "esp_log.h"
#include <cJSON.h>
#include <wifiManager.hpp>
#include <string>

CommandResult scanNetworksCommand(std::shared_ptr<DependencyRegistry> registry);

#endif