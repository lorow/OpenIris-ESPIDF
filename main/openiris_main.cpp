#include <stdio.h>
#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_psram.h"

#include <openiris_logo.hpp>
#include <wifiManager.hpp>
#include <ProjectConfig.hpp>
#include <LEDManager.hpp>
#include <MDNSManager.hpp>
#include <CameraManager.hpp>
#include <StreamServer.hpp>

#define BLINK_GPIO (gpio_num_t) CONFIG_BLINK_GPIO

static const char *TAG = "[MAIN]";

static void initNVSStorage()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

extern "C" void app_main(void)
{
    // porting plan:
    // port the wifi manager first. - worky!!!
    // get it connect to the network and setup an AP with hardcoded creds first -- connects. AP will be next
    // port the logo - done
    // port preferences lib -  DONE; prolly temporary
    // then port the config - done, needs todos done
    // State Management - done
    // then port the led manager as this will be fairly easy - done
    // then port the mdns stuff - done
    // then port the camera manager - in progress
    // then port the streaming stuff (web and uvc) - in progress

    // then add ADHOC and support for more networks in wifi manager
    // then port the async web server
    // then port the Elegant OTA stuff
    // then port the serial manager

    // TODO add this option
    // ProjectConfig deviceConfig("openiris", MDNS_HOSTNAME);
    ProjectConfig deviceConfig("openiris", "openiristracker");
    WiFiManager wifiManager;
    MDNSManager mdnsManager(deviceConfig);
    CameraManager cameraHandler(deviceConfig);
    StreamServer streamServer;

#ifdef CONFIG_USE_ILLUMNATIOR_PIN
    // LEDManager ledManager(BLINK_GPIO, ILLUMINATOR_PIN);
    LEDManager ledManager(BLINK_GPIO, 1);
#else
    LEDManager ledManager(BLINK_GPIO);
#endif

    Logo::printASCII();
    initNVSStorage();

    ledManager.setup();
    deviceConfig.load();
    wifiManager.Begin();
    mdnsManager.start();
    cameraHandler.setupCamera();
    streamServer.startStreamServer();

    while (1)
    {
        ledManager.handleLED();

        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
    }
}
