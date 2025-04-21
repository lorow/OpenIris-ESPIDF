#include "MDNSManager.hpp"

static const char *MDNS_MANAGER_TAG = "[MDNS MANAGER]";

MDNSManager::MDNSManager(std::shared_ptr<ProjectConfig> projectConfig, QueueHandle_t eventQueue) : projectConfig(projectConfig), eventQueue(eventQueue) {}

esp_err_t MDNSManager::start()
{
  const std::string &mdnsName = "_openiristracker";

  {
    SystemEvent event = {EventSource::MDNS, MDNSState_e::MDNSState_Starting};
    xQueueSend(this->eventQueue, &event, 10);
  }

  esp_err_t result = mdns_init();
  if (result != ESP_OK)
  {
    SystemEvent event = {EventSource::MDNS, MDNSState_e::MDNSState_Error};
    xQueueSend(this->eventQueue, &event, 10);
    ESP_LOGE(MDNS_MANAGER_TAG, "Failed to initialize mDNS server: %s", esp_err_to_name(result));
    return result;
  }

  auto mdnsConfig = projectConfig->getMDNSConfig();
  result = mdns_hostname_set(mdnsConfig.hostname.c_str());
  if (result != ESP_OK)
  {
    SystemEvent event = {EventSource::MDNS, MDNSState_e::MDNSState_Error};
    xQueueSend(this->eventQueue, &event, 10);
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
    SystemEvent event = {EventSource::MDNS, MDNSState_e::MDNSState_Error};
    xQueueSend(this->eventQueue, &event, 10);

    ESP_LOGE(MDNS_MANAGER_TAG, "Failed to set mDNS instance name: %s", esp_err_to_name(result));
    return result;
  }

  SystemEvent event = {EventSource::MDNS, MDNSState_e::MDNSState_Started};
  xQueueSend(this->eventQueue, &event, 10);

  return result;
}