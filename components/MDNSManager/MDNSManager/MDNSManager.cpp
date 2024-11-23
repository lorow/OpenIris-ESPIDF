#include "MDNSManager.hpp"

static const char *MDNS_MANAGER_TAG = "[MDNS MANAGER]";

MDNSManager::MDNSManager(std::shared_ptr<ProjectConfig> projectConfig) : projectConfig(projectConfig) {}

esp_err_t MDNSManager::start()
{
  const std::string &mdnsName = "_openiristracker";

  mdnsStateManager.setState(MDNSState_e::MDNSState_Starting);
  esp_err_t result = mdns_init();
  if (result != ESP_OK)
  {
    mdnsStateManager.setState(MDNSState_e::MDNSState_Error);
    ESP_LOGE(MDNS_MANAGER_TAG, "Failed to initialize mDNS server: %s", esp_err_to_name(result));
    return result;
  }

  auto mdnsConfig = projectConfig->getMDNSConfig();
  result = mdns_hostname_set(mdnsConfig.hostname.c_str());
  if (result != ESP_OK)
  {
    mdnsStateManager.setState(MDNSState_e::MDNSState_Error);
    ESP_LOGE(MDNS_MANAGER_TAG, "Failed to set hostname: %s", esp_err_to_name(result));
    return result;
  }

  mdns_txt_item_t serviceTxtData[3] = {
      {"stream_port", "80"},
      {"api_port", "81"},
  };

  result = mdns_service_add(nullptr, mdnsName.c_str(), "_tcp", 80, serviceTxtData, 2);

  result = mdns_service_instance_name_set(mdnsName.c_str(), "_tcp", mdnsName.c_str());
  if (result != ESP_OK)
  {
    mdnsStateManager.setState(MDNSState_e::MDNSState_Error);
    ESP_LOGE(MDNS_MANAGER_TAG, "Failed to set mDNS instance name: %s", esp_err_to_name(result));
    return result;
  }

  mdnsStateManager.setState(MDNSState_e::MDNSState_Started);

  return result;
}