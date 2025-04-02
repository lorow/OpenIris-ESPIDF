#include <stdio.h>
#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_camera.h"
#include "nvs_flash.h"
#include "esp_psram.h"

#include <openiris_logo.hpp>
#include <wifiManager.hpp>
#include <ProjectConfig.hpp>
#include <LEDManager.hpp>
#include <MDNSManager.hpp>
#include <CameraManager.hpp>
#include <WebSocketLogger.hpp>
#include <StreamServer.hpp>
#include <CommandManager.hpp>
#include <RestAPI.hpp>

#include <stdarg.h>

#ifdef CONFIG_WIRED_MODE
#include <UVCStream.hpp>
#endif

#define BLINK_GPIO (gpio_num_t) CONFIG_BLINK_GPIO

static const char *TAG = "[MAIN]";

WebSocketLogger webSocketLogger;
Preferences preferences;

auto deviceConfig = std::make_shared<ProjectConfig>(&preferences);
WiFiManager wifiManager(deviceConfig);
MDNSManager mdnsManager(deviceConfig);
std::shared_ptr<CameraManager> cameraHandler = std::make_shared<CameraManager>(deviceConfig);
StreamServer streamServer(80);

auto commandManager = std::make_shared<CommandManager>(deviceConfig, cameraHandler);
RestAPI restAPI("http://0.0.0.0:81", commandManager);

#ifdef CONFIG_WIRED_MODE
UVCStreamManager uvcStream;
#endif

#ifdef CONFIG_USE_ILLUMNATIOR_PIN
LEDManager ledManager(BLINK_GPIO, CONFIG_ILLUMINATOR_PIN);
#else
LEDManager ledManager(BLINK_GPIO);
#endif

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

int test_log(const char *format, va_list args)
{
    webSocketLogger.log_message(format, args);
    return vprintf(format, args);
}

extern "C" void app_main(void)
{
    // uvc plan

    // cleanup the logs - done
    // prepare the camera to be initialized with UVC - done?
    // debug uvc performance - done

    // porting plan:
    // port the wifi manager first. - worky!!!
    // port the logo - done
    // port preferences lib -  DONE; prolly temporary
    // then port the config - done, needs todos done
    // State Management - done
    // then port the led manager as this will be fairly easy - done
    // then port the mdns stuff - done
    // then port the camera manager - done
    // then port the streaming stuff (web and uvc) - done
    // then add ADHOC and support for more networks in wifi manager - done
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

    // simplify commands - a simple dependency injection + std::function should do it
    // something like
    // template<typename T>
    // void registerService(std::shared_pointer<T> service)
    // services[std::type_index(typeid(T))] = service;
    // where services is an std::unordered_map<std::type_index, std::shared_pointer<void>>;
    // I can then use like std::shared_ptr<T> resolve() { return services[typeid(T)]; } to get it in the command
    // which can be like a second parameter of the command, like std::function<void(DiContainer &diContainer, char* jsonPayload)>

    // simplify config - DONE
    // here I can decouple the loading, initializing and saving logic from the config class and move
    // that into the separate modules, and have the config class only act as a container

    // port serial manager
    // implement OTA stuff, but prepare it for future use
    // add endpoint to check firmware version
    // add firmware version somewhere
    // setup CI and building for other boards

    // then port the Elegant OTA stuff - I'll roll my own
    // finish todos, overhaul stuff a bit
    // maybe swich websocket logging to udp logging

    Logo::printASCII();
    initNVSStorage();

    esp_log_set_vprintf(&test_log);
    ledManager.setup();
    deviceConfig->load();
    wifiManager.Begin();
    mdnsManager.start();
    restAPI.begin();
    cameraHandler->setupCamera();
    // make sure the server runs on a separate core
    streamServer.startStreamServer();

#ifdef CONFIG_WIRED_MODE
    uvcStream.setup();
#endif
    while (1)
    {
        ledManager.handleLED();
        restAPI.poll();
        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
    }
}
