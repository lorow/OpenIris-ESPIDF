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
#include <main_globals.hpp>

#ifdef CONFIG_GENERAL_WIRED_MODE
#include <UVCStream.hpp>
#endif

#define BLINK_GPIO (gpio_num_t) CONFIG_LED_BLINK_GPIO
#define CONFIG_LED_C_PIN_GPIO (gpio_num_t) CONFIG_LED_EXTERNAL_GPIO

esp_timer_handle_t timerHandle;
QueueHandle_t eventQueue = xQueueCreate(10, sizeof(SystemEvent));
QueueHandle_t ledStateQueue = xQueueCreate(10, sizeof(uint32_t));

auto *stateManager = new StateManager(eventQueue, ledStateQueue);
auto dependencyRegistry = std::make_shared<DependencyRegistry>();
auto commandManager = std::make_shared<CommandManager>(dependencyRegistry);

WebSocketLogger webSocketLogger;
Preferences preferences;

std::shared_ptr<ProjectConfig> deviceConfig = std::make_shared<ProjectConfig>(&preferences);
auto wifiManager = std::make_shared<WiFiManager>(deviceConfig, eventQueue, stateManager);
MDNSManager mdnsManager(deviceConfig, eventQueue);

std::shared_ptr<CameraManager> cameraHandler = std::make_shared<CameraManager>(deviceConfig, eventQueue);
StreamServer streamServer(80, stateManager);

auto *restAPI = new RestAPI("http://0.0.0.0:81", commandManager);

#ifdef CONFIG_GENERAL_WIRED_MODE
UVCStreamManager uvcStream;
#endif

auto ledManager = std::make_shared<LEDManager>(BLINK_GPIO, CONFIG_LED_C_PIN_GPIO, ledStateQueue, deviceConfig);
auto *serialManager = new SerialManager(commandManager, &timerHandle, deviceConfig);

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

// New setup flow:
// 1. Device starts in setup mode (AP + Serial active)
// 2. User configures WiFi via serial commands
// 3. Device attempts WiFi connection while maintaining setup interfaces
// 4. Device reports connection status via serial
// 5. User explicitly starts streaming after verifying connectivity
void start_video_streaming(void *arg)
{
    // Get the stored device mode
    StreamingMode deviceMode = deviceConfig->getDeviceMode();

    // Check if WiFi is actually connected, not just configured
    bool hasWifiCredentials = !deviceConfig->getWifiConfigs().empty() || strcmp(CONFIG_WIFI_SSID, "") != 0;
    bool wifiConnected = (wifiManager->GetCurrentWiFiState() == WiFiState_e::WiFiState_Connected);

    if (deviceMode == StreamingMode::UVC)
    {
#ifdef CONFIG_GENERAL_WIRED_MODE
        ESP_LOGI("[MAIN]", "Starting UVC streaming mode.");
        ESP_LOGI("[MAIN]", "Initializing UVC hardware...");
        esp_err_t ret = uvcStream.setup();
        if (ret != ESP_OK)
        {
            ESP_LOGE("[MAIN]", "Failed to initialize UVC: %s", esp_err_to_name(ret));
            return;
        }
        uvcStream.start();
#else
        ESP_LOGE("[MAIN]", "UVC mode selected but the board likely does not support it.");
        ESP_LOGI("[MAIN]", "Falling back to WiFi mode if credentials available");
        deviceMode = StreamingMode::WIFI;
#endif
    }

    if ((deviceMode == StreamingMode::WIFI || deviceMode == StreamingMode::AUTO) && hasWifiCredentials && wifiConnected)
    {
        ESP_LOGI("[MAIN]", "Starting WiFi streaming mode.");
        streamServer.startStreamServer();
    }
    else
    {
        if (hasWifiCredentials && !wifiConnected)
        {
            ESP_LOGE("[MAIN]", "WiFi credentials configured but not connected. Try connecting first.");
        }
        else
        {
            ESP_LOGE("[MAIN]", "No streaming mode available. Please configure WiFi.");
        }
        return;
    }

    ESP_LOGI("[MAIN]", "Streaming started successfully.");

    // Optionally disable serial manager after explicit streaming start
    if (arg != nullptr)
    {
        ESP_LOGI("[MAIN]", "Disabling setup interfaces after streaming start.");
        const auto serialTaskHandle = static_cast<TaskHandle_t>(arg);
        disable_serial_manager_task(serialTaskHandle);
    }
}

// Manual streaming activation - no timer needed
void activate_streaming(TaskHandle_t serialTaskHandle = nullptr)
{
    start_video_streaming(serialTaskHandle);
}

// Callback for automatic startup after delay
void startup_timer_callback(void *arg)
{
    ESP_LOGI("[MAIN]", "Startup timer fired, startupCommandReceived=%s, startupPaused=%s",
             getStartupCommandReceived() ? "true" : "false",
             getStartupPaused() ? "true" : "false");

    if (!getStartupCommandReceived() && !getStartupPaused())
    {
        ESP_LOGI("[MAIN]", "No command received during startup delay, proceeding with automatic mode startup");

        // Get the stored device mode
        StreamingMode deviceMode = deviceConfig->getDeviceMode();
        ESP_LOGI("[MAIN]", "Stored device mode: %d", (int)deviceMode);

        // Get the serial manager handle to disable it after streaming starts
        TaskHandle_t *serialHandle = getSerialManagerHandle();
        TaskHandle_t serialTaskHandle = (serialHandle && *serialHandle) ? *serialHandle : nullptr;

        if (deviceMode == StreamingMode::WIFI || deviceMode == StreamingMode::AUTO)
        {
            // For WiFi mode, check if we have credentials and are connected
            bool hasWifiCredentials = !deviceConfig->getWifiConfigs().empty() || strcmp(CONFIG_WIFI_SSID, "") != 0;
            bool wifiConnected = (wifiManager->GetCurrentWiFiState() == WiFiState_e::WiFiState_Connected);

            ESP_LOGI("[MAIN]", "WiFi check - hasCredentials: %s, connected: %s",
                     hasWifiCredentials ? "true" : "false",
                     wifiConnected ? "true" : "false");

            if (hasWifiCredentials && wifiConnected)
            {
                ESP_LOGI("[MAIN]", "Starting WiFi streaming automatically");
                activate_streaming(serialTaskHandle);
            }
            else if (hasWifiCredentials && !wifiConnected)
            {
                ESP_LOGI("[MAIN]", "WiFi credentials exist but not connected, waiting...");
                // Could retry connection here
            }
            else
            {
                ESP_LOGI("[MAIN]", "No WiFi credentials, staying in setup mode");
            }
        }
        else if (deviceMode == StreamingMode::UVC)
        {
#ifdef CONFIG_GENERAL_WIRED_MODE
            ESP_LOGI("[MAIN]", "Starting UVC streaming automatically");
            activate_streaming(serialTaskHandle);
#else
            ESP_LOGE("[MAIN]", "UVC mode selected but CONFIG_GENERAL_WIRED_MODE not enabled in build!");
            ESP_LOGI("[MAIN]", "Device will stay in setup mode. Enable CONFIG_GENERAL_WIRED_MODE and rebuild.");
#endif
        }
        else
        {
            ESP_LOGI("[MAIN]", "Unknown device mode: %d", (int)deviceMode);
        }
    }
    else
    {
        if (getStartupPaused())
        {
            ESP_LOGI("[MAIN]", "Startup paused, staying in heartbeat mode");
        }
        else
        {
            ESP_LOGI("[MAIN]", "Command received during startup, staying in heartbeat mode");
        }
    }

    // Delete the timer after it fires
    esp_timer_delete(timerHandle);
    timerHandle = nullptr;
}

extern "C" void app_main(void)
{
    dependencyRegistry->registerService<ProjectConfig>(DependencyType::project_config, deviceConfig);
    dependencyRegistry->registerService<CameraManager>(DependencyType::camera_manager, cameraHandler);
    dependencyRegistry->registerService<WiFiManager>(DependencyType::wifi_manager, wifiManager);
    dependencyRegistry->registerService<LEDManager>(DependencyType::led_manager, ledManager);
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

    // esp_log_set_vprintf(&websocket_logger);
    Logo::printASCII();
    initNVSStorage();
    deviceConfig->load();
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
        ledManager.get(),
        3,
        nullptr);

    serialManager->setup();

    static TaskHandle_t serialManagerHandle = nullptr;
    // Pass address of variable so xTaskCreate() stores the actual task handle
    xTaskCreate(
        HandleSerialManagerTask,
        "HandleSerialManagerTask",
        1024 * 6,
        serialManager,
        1, // we only rely on the serial manager during provisioning, we can run it slower
        &serialManagerHandle);

    wifiManager->Begin();
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

    // New flow: Device starts with a 20-second delay before automatic mode startup
    ESP_LOGI("[MAIN]", "=====================================");
    ESP_LOGI("[MAIN]", "STARTUP: 20-SECOND DELAY MODE ACTIVE");
    ESP_LOGI("[MAIN]", "=====================================");
    ESP_LOGI("[MAIN]", "Device will wait 20 seconds for commands...");

    // Create a one-shot timer for 20 seconds
    const esp_timer_create_args_t startup_timer_args = {
        .callback = &startup_timer_callback,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "startup_timer",
        .skip_unhandled_events = false};

    ESP_ERROR_CHECK(esp_timer_create(&startup_timer_args, &timerHandle));
    ESP_ERROR_CHECK(esp_timer_start_once(timerHandle, CONFIG_GENERAL_UVC_DELAY * 1000000));
    ESP_LOGI("[MAIN]", "Started 20-second startup timer");
    ESP_LOGI("[MAIN]", "Send any command within 20 seconds to enter heartbeat mode");
    setSerialManagerHandle(&serialManagerHandle);
}