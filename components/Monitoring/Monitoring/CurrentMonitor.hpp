#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include "sdkconfig.h"

class CurrentMonitor {
public:
    CurrentMonitor();
    ~CurrentMonitor() = default;

    void setup();
    void sampleOnce();

    // Returns filtered voltage in millivolts at shunt (after dividing by gain)
    int getFilteredMillivolts() const { return filtered_mv_; }
    // Returns current in milliamps computed as Vshunt[mV] / R[mΩ]
    float getCurrentMilliAmps() const;

    // convenience: combined sampling and compute; returns mA
    float pollAndGetMilliAmps();

    // Whether monitoring is enabled by Kconfig
    static constexpr bool isEnabled()
    {
    #ifdef CONFIG_MONITORING_LED_CURRENT
        return true;
    #else
        return false;
    #endif
    }

private:
#if CONFIG_MONITORING_LED_CURRENT
    void init_adc();
    int read_mv_once();
    int gpio_to_adc_channel(int gpio);
#endif

    int filtered_mv_ = 0;
    int sample_sum_ = 0;
    std::vector<int> samples_;
    size_t sample_idx_ = 0;
    size_t sample_count_ = 0;
};
