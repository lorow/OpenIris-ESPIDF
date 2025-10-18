#include "MonitoringManager.hpp"
#include <esp_log.h>
#include "sdkconfig.h"

static const char* TAG_MM = "[MonitoringManager]";

void MonitoringManager::setup()
{
#if CONFIG_MONITORING_LED_CURRENT
    cm_.setup();
    ESP_LOGI(TAG_MM, "Monitoring enabled. Interval=%dms, Samples=%d, Gain=%d, R=%dmΩ",
             CONFIG_MONITORING_LED_INTERVAL_MS,
             CONFIG_MONITORING_LED_SAMPLES,
             CONFIG_MONITORING_LED_GAIN,
             CONFIG_MONITORING_LED_SHUNT_MILLIOHM);
#else
    ESP_LOGI(TAG_MM, "Monitoring disabled by Kconfig");
#endif
}

void MonitoringManager::start()
{
#if CONFIG_MONITORING_LED_CURRENT
    if (task_ == nullptr)
    {
        xTaskCreate(&MonitoringManager::taskEntry, "MonitoringTask", 2048, this, 1, &task_);
    }
#endif
}

void MonitoringManager::stop()
{
    if (task_)
    {
        TaskHandle_t toDelete = task_;
        task_ = nullptr;
        vTaskDelete(toDelete);
    }
}

void MonitoringManager::taskEntry(void* arg)
{
    static_cast<MonitoringManager*>(arg)->run();
}

void MonitoringManager::run()
{
#if CONFIG_MONITORING_LED_CURRENT
    while (true)
    {
        float ma = cm_.pollAndGetMilliAmps();
        last_current_ma_.store(ma);
        vTaskDelay(pdMS_TO_TICKS(CONFIG_MONITORING_LED_INTERVAL_MS));
    }
#else
    vTaskDelete(nullptr);
#endif
}
