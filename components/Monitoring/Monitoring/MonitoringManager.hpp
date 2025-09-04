#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <atomic>
#include "CurrentMonitor.hpp"

class MonitoringManager {
public:
    MonitoringManager() = default;
    ~MonitoringManager() = default;

    void setup();
    void start();
    void stop();

    // Latest filtered current in mA
    float getCurrentMilliAmps() const { return last_current_ma_.load(); }

private:
    static void taskEntry(void* arg);
    void run();

    TaskHandle_t task_{nullptr};
    std::atomic<float> last_current_ma_{0.0f};
    CurrentMonitor cm_;
};
