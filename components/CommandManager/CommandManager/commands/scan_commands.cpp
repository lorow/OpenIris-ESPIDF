#include "scan_commands.hpp"

CommandResult scanNetworksCommand(std::shared_ptr<DependencyRegistry> registry)
{
    auto wifiManager = registry->resolve<WiFiManager>(DependencyType::wifi_manager);
    if (!wifiManager)
    {
        return CommandResult::getErrorResult("WiFiManager not available");
    }

    auto networks = wifiManager->ScanNetworks();

    cJSON *root = cJSON_CreateObject();
    cJSON *networksArray = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "networks", networksArray);

    for (const auto &network : networks)
    {
        cJSON *networkObject = cJSON_CreateObject();
        cJSON_AddStringToObject(networkObject, "ssid", network.ssid.c_str());
        cJSON_AddNumberToObject(networkObject, "channel", network.channel);
        cJSON_AddNumberToObject(networkObject, "rssi", network.rssi);
        char mac_str[18];
        sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
                network.mac[0], network.mac[1], network.mac[2],
                network.mac[3], network.mac[4], network.mac[5]);
        cJSON_AddStringToObject(networkObject, "mac_address", mac_str);
        cJSON_AddNumberToObject(networkObject, "auth_mode", network.auth_mode);
        cJSON_AddItemToArray(networksArray, networkObject);
    }

    char *json_string = cJSON_PrintUnformatted(root);
    printf("%s\n", json_string);
    cJSON_Delete(root);
    free(json_string);

    return CommandResult::getSuccessResult("Networks scanned");
}