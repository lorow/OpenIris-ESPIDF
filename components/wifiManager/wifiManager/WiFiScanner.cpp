#include "WiFiScanner.hpp"
#include <cstring>

static const char *TAG = "WiFiScanner";

WiFiScanner::WiFiScanner() {}

void WiFiScanner::scanResultCallback(void *arg, esp_event_base_t event_base,
                                     int32_t event_id, void *event_data)
{
    auto *scanner = static_cast<WiFiScanner *>(arg);
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE)
    {
        uint16_t ap_count = 0;
        esp_wifi_scan_get_ap_num(&ap_count);

        if (ap_count == 0)
        {
            ESP_LOGI(TAG, "No access points found");
            return;
        }

        wifi_ap_record_t *ap_records = new wifi_ap_record_t[ap_count];
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_records));

        scanner->networks.clear();
        for (uint16_t i = 0; i < ap_count; i++)
        {
            WiFiNetwork network;
            network.ssid = std::string(reinterpret_cast<char *>(ap_records[i].ssid));
            network.channel = ap_records[i].primary;
            network.rssi = ap_records[i].rssi;
            memcpy(network.mac, ap_records[i].bssid, 6);
            network.auth_mode = ap_records[i].authmode;

            scanner->networks.push_back(network);
        }

        delete[] ap_records;
        ESP_LOGI(TAG, "Found %d access points", ap_count);
    }
}

// todo this is garbage
std::vector<WiFiNetwork> WiFiScanner::scanNetworks()
{
    std::vector<WiFiNetwork> scan_results;

    // Check if WiFi is initialized
    wifi_mode_t mode;
    esp_err_t err = esp_wifi_get_mode(&mode);
    if (err == ESP_ERR_WIFI_NOT_INIT)
    {
        ESP_LOGE(TAG, "WiFi not initialized");
        return scan_results;
    }

    // Give WiFi more time to be ready
    vTaskDelay(pdMS_TO_TICKS(500));

    // Stop any ongoing scan
    esp_wifi_scan_stop();

    // Try sequential channel scanning as a workaround
    bool try_sequential_scan = true; // Enable sequential scan

    if (!try_sequential_scan)
    {
        // Normal all-channel scan
        wifi_scan_config_t scan_config = {
            .ssid = nullptr,
            .bssid = nullptr,
            .channel = 0, // 0 means scan all channels
            .show_hidden = true,
            .scan_type = WIFI_SCAN_TYPE_ACTIVE, // Active scan
            .scan_time = {
                .active = {
                    .min = 120, // Min per channel
                    .max = 300  // Max per channel
                },
                .passive = 360},
            .home_chan_dwell_time = 0, // 0 for default
            .channel_bitmap = 0        // 0 for all channels
        };

        err = esp_wifi_scan_start(&scan_config, false);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to start scan: %s", esp_err_to_name(err));
            return scan_results;
        }
    }
    else
    {
        // Sequential channel scan - scan each channel individually
        std::vector<wifi_ap_record_t> all_records;

        for (uint8_t ch = 1; ch <= 13; ch++)
        {
            wifi_scan_config_t scan_config = {
                .ssid = nullptr,
                .bssid = nullptr,
                .channel = ch,
                .show_hidden = true,
                .scan_type = WIFI_SCAN_TYPE_ACTIVE,
                .scan_time = {
                    .active = {
                        .min = 100,
                        .max = 200},
                    .passive = 300},
                .home_chan_dwell_time = 0,
                .channel_bitmap = 0};

            err = esp_wifi_scan_start(&scan_config, true); // Blocking scan
            if (err == ESP_OK)
            {
                uint16_t ch_count = 0;
                esp_wifi_scan_get_ap_num(&ch_count);
                if (ch_count > 0)
                {
                    wifi_ap_record_t *ch_records = new wifi_ap_record_t[ch_count];
                    if (esp_wifi_scan_get_ap_records(&ch_count, ch_records) == ESP_OK)
                    {
                        for (uint16_t i = 0; i < ch_count; i++)
                        {
                            all_records.push_back(ch_records[i]);
                        }
                    }
                    delete[] ch_records;
                }
            }
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        // Process all collected records
        for (const auto &record : all_records)
        {
            WiFiNetwork network;
            network.ssid = std::string(reinterpret_cast<const char *>(record.ssid));
            network.channel = record.primary;
            network.rssi = record.rssi;
            memcpy(network.mac, record.bssid, 6);
            network.auth_mode = record.authmode;
            scan_results.push_back(network);
        }

        // Skip the normal result processing
        return scan_results;
    }

    // Wait for scan completion with timeout
    int timeout_ms = 15000; // 15 second timeout
    int elapsed_ms = 0;

    while (elapsed_ms < timeout_ms)
    {
        uint16_t temp_count = 0;
        esp_err_t count_err = esp_wifi_scan_get_ap_num(&temp_count);

        if (count_err == ESP_OK)
        {
            // Wait a bit longer after finding networks to ensure scan is complete
            if (temp_count > 0 && elapsed_ms > 5000)
            {
                break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(200));
        elapsed_ms += 200;
    }

    if (elapsed_ms >= timeout_ms)
    {
        ESP_LOGE(TAG, "Scan timeout after %d ms", timeout_ms);
        esp_wifi_scan_stop();
        return scan_results;
    }

    // Get scan results
    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);

    if (ap_count == 0)
    {
        ESP_LOGI(TAG, "No access points found");
        return scan_results;
    }

    wifi_ap_record_t *ap_records = new wifi_ap_record_t[ap_count];
    err = esp_wifi_scan_get_ap_records(&ap_count, ap_records);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get scan records: %s", esp_err_to_name(err));
        delete[] ap_records;
        return scan_results;
    }

    // Build the results vector and track channels found
    bool channels_found[15] = {false}; // Track channels 0-14

    for (uint16_t i = 0; i < ap_count; i++)
    {
        WiFiNetwork network;
        network.ssid = std::string(reinterpret_cast<char *>(ap_records[i].ssid));
        network.channel = ap_records[i].primary;
        network.rssi = ap_records[i].rssi;
        memcpy(network.mac, ap_records[i].bssid, 6);
        network.auth_mode = ap_records[i].authmode;

        if (network.channel <= 14)
        {
            channels_found[network.channel] = true;
        }

        scan_results.push_back(network);
    }

    delete[] ap_records;
    ESP_LOGI(TAG, "Found %d access points", ap_count);

    return scan_results;
}