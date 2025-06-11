#pragma once
#ifndef WIFI_SCANNER_HPP
#define WIFI_SCANNER_HPP

#include <vector>
#include <string>
#include "esp_wifi.h"
#include "esp_log.h"

struct WiFiNetwork {
    std::string ssid;
    uint8_t channel;
    int8_t rssi;
    uint8_t mac[6];
    wifi_auth_mode_t auth_mode;
};

class WiFiScanner {
public:
    WiFiScanner();
    std::vector<WiFiNetwork> scanNetworks();
    static void scanResultCallback(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

private:
    static constexpr char const* TAG = "WiFiScanner";
    std::vector<WiFiNetwork> networks;
};

#endif