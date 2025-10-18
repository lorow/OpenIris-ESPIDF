#include "CurrentMonitor.hpp"
#include <esp_log.h>
#include <cmath>

#if CONFIG_MONITORING_LED_CURRENT
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#endif

static const char *TAG_CM = "[CurrentMonitor]";

CurrentMonitor::CurrentMonitor()
{
#if CONFIG_MONITORING_LED_CURRENT
    samples_.assign(CONFIG_MONITORING_LED_SAMPLES, 0);
#endif
}

void CurrentMonitor::setup()
{
#if CONFIG_MONITORING_LED_CURRENT
    init_adc();
#else
    ESP_LOGI(TAG_CM, "LED current monitoring disabled");
#endif
}

float CurrentMonitor::getCurrentMilliAmps() const
{
#if CONFIG_MONITORING_LED_CURRENT
    const int shunt_milliohm = CONFIG_MONITORING_LED_SHUNT_MILLIOHM; // mΩ
    if (shunt_milliohm <= 0)
        return 0.0f;
    // Physically correct scaling:
    // I[mA] = 1000 * Vshunt[mV] / R[mΩ]
    return (1000.0f * static_cast<float>(filtered_mv_)) / static_cast<float>(shunt_milliohm);
#else
    return 0.0f;
#endif
}

float CurrentMonitor::pollAndGetMilliAmps()
{
    sampleOnce();
    return getCurrentMilliAmps();
}

void CurrentMonitor::sampleOnce()
{
#if CONFIG_MONITORING_LED_CURRENT
    int mv = read_mv_once();
    // Divide by analog gain/divider factor to get shunt voltage
    if (CONFIG_MONITORING_LED_GAIN > 0)
        mv = mv / CONFIG_MONITORING_LED_GAIN;

    // Moving average over N samples
    if (samples_.empty())
    {
        samples_.assign(CONFIG_MONITORING_LED_SAMPLES, 0);
        sample_sum_ = 0;
        sample_idx_ = 0;
        sample_count_ = 0;
    }

    sample_sum_ -= samples_[sample_idx_];
    samples_[sample_idx_] = mv;
    sample_sum_ += mv;
    sample_idx_ = (sample_idx_ + 1) % samples_.size();
    if (sample_count_ < samples_.size())
        sample_count_++;

    filtered_mv_ = sample_sum_ / static_cast<int>(sample_count_ > 0 ? sample_count_ : 1);
#else
    (void)0;
#endif
}

#if CONFIG_MONITORING_LED_CURRENT

static adc_oneshot_unit_handle_t s_adc_handle = nullptr;
static adc_cali_handle_t s_cali_handle = nullptr;
static bool s_cali_inited = false;
static adc_channel_t s_channel;
static adc_unit_t s_unit;

void CurrentMonitor::init_adc()
{
    // Derive ADC unit/channel from GPIO
    int gpio = CONFIG_MONITORING_LED_ADC_GPIO;

    esp_err_t err;
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    err = adc_oneshot_new_unit(&unit_cfg, &s_adc_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_CM, "adc_oneshot_new_unit failed: %s", esp_err_to_name(err));
        return;
    }

    // Try to map GPIO to ADC channel automatically if helper exists; otherwise guess for ESP32-S3 ADC1
#ifdef ADC1_GPIO1_CHANNEL
    (void)0; // placeholder for potential future macros
#endif

    // Use IO-to-channel helper where available
#ifdef CONFIG_IDF_TARGET_ESP32S3
    // ESP32-S3: ADC1 channels on GPIO1..GPIO10 map to CH0..CH9
    if (gpio >= 1 && gpio <= 10)
    {
        s_unit = ADC_UNIT_1;
        s_channel = static_cast<adc_channel_t>(gpio - 1);
    }
    else
    {
        ESP_LOGW(TAG_CM, "Configured GPIO %d may not be ADC-capable on ESP32-S3", gpio);
        s_unit = ADC_UNIT_1;
        s_channel = ADC_CHANNEL_0;
    }
#else
    // Fallback: assume ADC1 CH0
    s_unit = ADC_UNIT_1;
    s_channel = ADC_CHANNEL_0;
#endif

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    err = adc_oneshot_config_channel(s_adc_handle, s_channel, &chan_cfg);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_CM, "adc_oneshot_config_channel failed: %s", esp_err_to_name(err));
    }

    // Calibration using curve fitting if available
    adc_cali_curve_fitting_config_t cal_cfg = {
        .unit_id = s_unit,
        .atten = chan_cfg.atten,
        .bitwidth = chan_cfg.bitwidth,
    };
    if (adc_cali_create_scheme_curve_fitting(&cal_cfg, &s_cali_handle) == ESP_OK)
    {
        s_cali_inited = true;
        ESP_LOGI(TAG_CM, "ADC calibration initialized (curve fitting)");
    }
    else
    {
        s_cali_inited = false;
        ESP_LOGW(TAG_CM, "ADC calibration not available; using raw-to-mV approximation");
    }
}

int CurrentMonitor::read_mv_once()
{
    if (!s_adc_handle)
        return 0;
    int raw = 0;
    if (adc_oneshot_read(s_adc_handle, s_channel, &raw) != ESP_OK)
        return 0;

    int mv = 0;
    if (s_cali_inited)
    {
        if (adc_cali_raw_to_voltage(s_cali_handle, raw, &mv) != ESP_OK)
            mv = 0;
    }
    else
    {
        // Very rough fallback for 11dB attenuation
        // Typical full-scale ~2450mV at raw max 4095 (12-bit). IDF defaults may vary.
        mv = (raw * 2450) / 4095;
    }
    return mv;
}

#endif // CONFIG_MONITORING_LED_CURRENT
