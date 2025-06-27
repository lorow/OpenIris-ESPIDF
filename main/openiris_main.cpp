#include <cstdio>
#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "nvs_flash.h"

#include <openiris_logo.hpp>
#include <wifiManager.hpp>
#include <ProjectConfig.hpp>
#include <StateManager.hpp>
#include <LEDManager.hpp>
#include <MDNSManager.hpp>
#include <CameraManager.hpp>
#include <WebSocketLogger.hpp>
#include <StreamServer.hpp>
#include <CommandManager.hpp>
#include <SerialManager.hpp>
#include <RestAPI.hpp>

#ifdef CONFIG_WIRED_MODE
#include <UVCStream.hpp>
#endif

#define BLINK_GPIO (gpio_num_t) CONFIG_BLINK_GPIO
#define CONFIG_LED_C_PIN_GPIO (gpio_num_t) CONFIG_LED_C_PIN

esp_timer_handle_t timerHandle;
QueueHandle_t eventQueue = xQueueCreate(10, sizeof(SystemEvent));
QueueHandle_t ledStateQueue = xQueueCreate(10, sizeof(uint32_t));

auto *stateManager = new StateManager(eventQueue, ledStateQueue);
auto dependencyRegistry = std::make_shared<DependencyRegistry>();
auto commandManager = std::make_shared<CommandManager>(dependencyRegistry);

WebSocketLogger webSocketLogger;
Preferences preferences;

std::shared_ptr<ProjectConfig> deviceConfig = std::make_shared<ProjectConfig>(&preferences);
WiFiManager wifiManager(deviceConfig, eventQueue, stateManager);
MDNSManager mdnsManager(deviceConfig, eventQueue);

std::shared_ptr<CameraManager> cameraHandler = std::make_shared<CameraManager>(deviceConfig, eventQueue);
StreamServer streamServer(80, stateManager);

auto *restAPI = new RestAPI("http://0.0.0.0:81", commandManager);

#ifdef CONFIG_WIRED_MODE
UVCStreamManager uvcStream;
#endif

auto *ledManager = new LEDManager(BLINK_GPIO, CONFIG_LED_C_PIN_GPIO, ledStateQueue);
auto *serialManager = new SerialManager(commandManager, &timerHandle);

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

int websocket_logger(const char *format, va_list args)
{
    webSocketLogger.log_message(format, args);
    return vprintf(format, args);
}

void disable_serial_manager_task(TaskHandle_t serialManagerHandle)
{
    vTaskDelete(serialManagerHandle);
}

// the idea here is pretty simple.
// After setting everything up, we start a 30s timer with this as a callback
// if we get anything on the serial, we stop the timer and reset it after the commands are done
// this is done to ensure the user has enough time to configure the board if need be
void start_video_streaming(void *arg)
{
    // if we're in auto-mode, we can decide which streaming helper to start based on the
    // presence of Wi-Fi credentials
    ESP_LOGI("[MAIN]", "Setup window expired, starting streaming services, quitting serial manager.");
    switch (deviceConfig->getDeviceMode().mode)
    {
    case StreamingMode::AUTO:
        if (!deviceConfig->getWifiConfigs().empty() || strcmp(CONFIG_WIFI_SSID, "") != 0)
        {
            // todo make sure the server runs on a separate core
            ESP_LOGI("[MAIN]", "WiFi setup detected, starting WiFi streaming.");
            streamServer.startStreamServer();
        }
        else
        {
            ESP_LOGI("[MAIN]", "UVC setup detected, starting UVC streaming.");
            uvcStream.setup();
        }
        break;
    case StreamingMode::UVC:
        ESP_LOGI("[MAIN]", "Device set to UVC Mode, starting UVC streaming.");
        uvcStream.setup();
        break;
    case StreamingMode::WIFI:
        ESP_LOGI("[MAIN]", "Device set to Wi-Fi mode, starting WiFi streaming.");
        streamServer.startStreamServer();
        break;
    }

    const auto serialTaskHandle = static_cast<TaskHandle_t>(arg);
    disable_serial_manager_task(serialTaskHandle);
}

esp_timer_handle_t createStartVideoStreamingTimer(void *pvParameter)
{
    esp_timer_handle_t handle;
    const esp_timer_create_args_t args = {
        .callback = &start_video_streaming,
        .arg = pvParameter,
        .name = "startVideoStreaming"};

    if (const auto result = esp_timer_create(&args, &handle); result != ESP_OK)
    {
        ESP_LOGE("[MAIN]", "Failed to create timer: %s", esp_err_to_name(result));
    }

    return handle;
}

extern "C" void app_main(void)
{
    TaskHandle_t *serialManagerHandle = nullptr;
    dependencyRegistry->registerService<ProjectConfig>(DependencyType::project_config, deviceConfig);
    dependencyRegistry->registerService<CameraManager>(DependencyType::camera_manager, cameraHandler);
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

    // simplify commands - a simple dependency injection + std::function should do it - DONE
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

    // rethink led manager - we need to move the state change sending into a queue and rethink the state lighting logic - DONE
    // also, the entire led manager needs to be moved to a task - DONE
    // with that, I couuld use vtaskdelayuntil to advance and display states - DONE
    // and with that, I should rethink how state management works - DONE

    // rethink state management - DONE

    // port serial manager - DONE
    // instead of the UVCCDC thing - give the board 30s for serial commands and then determine if we should reboot into UVC - DONE

    // add endpoint to check firmware version
    // add firmware version somewhere
    // setup CI and building for other boards
    // finish todos, overhaul stuff a bit

    Logo::printASCII();
    initNVSStorage();

    esp_log_set_vprintf(&websocket_logger);
    ledManager->setup();

    xTaskCreate(
        HandleStateManagerTask,
        "HandleLEDDisplayTask",
        1024 * 2,
        stateManager,
        3,
        nullptr // it's fine for us not get a handle back, we don't need it
    );

    xTaskCreate(
        HandleLEDDisplayTask,
        "HandleLEDDisplayTask",
        1024 * 2,
        ledManager,
        3,
        nullptr);

    deviceConfig->load();
    serialManager->setup();

    xTaskCreate(
        HandleSerialManagerTask,
        "HandleSerialManagerTask",
        1024 * 6,
        serialManager,
        1, // we only rely on the serial manager during provisioning, we can run it slower
        serialManagerHandle);

    wifiManager.Begin();
    mdnsManager.start();
    restAPI->begin();
    cameraHandler->setupCamera();

    xTaskCreate(
        HandleRestAPIPollTask,
        "HandleRestAPIPollTask",
        1024 * 2,
        restAPI,
        1, // it's the rest API, we only serve commands over it so we don't really need a higher priority
        nullptr);

    timerHandle = createStartVideoStreamingTimer(serialManagerHandle);
    if (timerHandle != nullptr)
    {
        esp_timer_start_once(timerHandle, 30000000); // 30s
    }
}
