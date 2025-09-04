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
test
#ifdef CONFIG_GENERAL_DEFAULT_WIRED_MODE
#include <UVCStream.hpp>
#endif

#define BLINK_GPIO (gpio_num_t) CONFIG_LED_BLINK_GPIO
#define CONFIG_LED_C_PIN_GPIO (gpio_num_t) CONFIG_LED_EXTERNAL_GPIO

TaskHandle_t serialManagerHandle;

esp_timer_handle_t timerHandle;
QueueHandle_t eventQueue = xQueueCreate(10, sizeof(SystemEvent));
QueueHandle_t ledStateQueue = xQueueCreate(10, sizeof(uint32_t));
QueueHandle_t cdcMessageQueue = xQueueCreate(3, sizeof(cdc_command_packet_t));

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

#ifdef CONFIG_GENERAL_DEFAULT_WIRED_MODE
UVCStreamManager uvcStream;
#endif

auto ledManager = std::make_shared<LEDManager>(BLINK_GPIO, CONFIG_LED_C_PIN_GPIO, ledStateQueue, deviceConfig);
auto *serialManager = new SerialManager(commandManager, &timerHandle, deviceConfig);

void startWiFiMode(bool shouldCloseSerialManager);
void startWiredMode(bool shouldCloseSerialManager);

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

void launch_streaming()
{
    // Note, when switching and later right away activating UVC mode when we were previously in WiFi or Auto mode, the WiFi
    // utilities will still be running since we've launched them with startAutoMode() -> startWiFiMode()
    // we could add detection of this case, but it's probably not worth it since the next start of the device literally won't launch them
    // and we're telling folks to just reboot the device anyway
    // same case goes for when switching from UVC to WiFi

    StreamingMode deviceMode = deviceConfig->getDeviceMode();
    // if we've changed the mode from auto to something else, we can clean up serial manager
    // either the API endpoints or CDC will take care of further configuration
    if (deviceMode == StreamingMode::WIFI)
    {
        startWiFiMode(true);
    }
    else if (deviceMode == StreamingMode::UVC)
    {
        startWiredMode(true);
    }
    else if (deviceMode == StreamingMode::AUTO)
    {
        // we're still in auto, the user didn't select anything yet, let's give a bit of time for them to make a choice
        ESP_LOGI("[MAIN]", "No mode was selected, staying in AUTO mode. WiFi streaming will be enabled still. \nPlease select another mode if you'd like.");
    }
    else
    {
        ESP_LOGI("[MAIN]", "Unknown device mode: %d", (int)deviceMode);
    }
}

// Callback for automatic startup after delay
void startup_timer_callback(void *arg)
{
    ESP_LOGI("[MAIN]", "Startup timer fired, startupCommandReceived=%s, startupPaused=%s",
             getStartupCommandReceived() ? "true" : "false",
             getStartupPaused() ? "true" : "false");

    if (!getStartupCommandReceived() && !getStartupPaused())
    {
        launch_streaming();
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

// Manual streaming activation
// We'll clean up the timer and handle streaming setup if called by a command
void force_activate_streaming()
{
    // Delete the timer before it fires
    // since we've got called manually
    esp_timer_delete(timerHandle);
    timerHandle = nullptr;
    launch_streaming();
}

void startWiredMode(bool shouldCloseSerialManager)
{
#ifndef CONFIG_GENERAL_DEFAULT_WIRED_MODE
    ESP_LOGE("[MAIN]", "UVC mode selected but the board likely does not support it.");
    ESP_LOGI("[MAIN]", "Falling back to WiFi mode if credentials available");
    deviceMode = StreamingMode::WIFI;
    startWiFiMode();
#else
    ESP_LOGI("[MAIN]", "Starting UVC streaming mode.");
    if (shouldCloseSerialManager)
    {
        ESP_LOGI("[MAIN]", "Closing serial manager task.");
        vTaskDelete(serialManagerHandle);
    }

    ESP_LOGI("[MAIN]", "Shutting down serial manager, CDC will take over in a bit.");
    serialManager->shutdown();

    ESP_LOGI("[MAIN]", "Serial driver uninstalled");
    // Leaving a small gap for the host to see COM disappear
    vTaskDelay(pdMS_TO_TICKS(200));
    setUsbHandoverDone(true);

    ESP_LOGI("[MAIN]", "Setting up UVC Streamer");

    esp_err_t ret = uvcStream.setup();
    if (ret != ESP_OK)
    {
        ESP_LOGE("[MAIN]", "Failed to initialize UVC: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI("[MAIN]", "Starting CDC Serial Manager Task");
    xTaskCreate(
        HandleCDCSerialManagerTask,
        "HandleCDCSerialManagerTask",
        1024 * 6,
        commandManager.get(),
        1,
        nullptr);

    ESP_LOGI("[MAIN]", "Starting UVC streaming");

    uvcStream.start();
    ESP_LOGI("[MAIN]", "UVC streaming started");
#endif
}

void startWiFiMode(bool shouldCloseSerialManager)
{
    ESP_LOGI("[MAIN]", "Starting WiFi streaming mode.");
    if (shouldCloseSerialManager)
    {
        ESP_LOGI("[MAIN]", "Closing serial manager task.");
        vTaskDelete(serialManagerHandle);
    }

    wifiManager->Begin();
    mdnsManager.start();
    restAPI->begin();

    xTaskCreate(
        HandleRestAPIPollTask,
        "HandleRestAPIPollTask",
        1024 * 2,
        restAPI,
        1, // it's the rest API, we only serve commands over it so we don't really need a higher priority
        nullptr);
}

void startSetupMode()
{
    // If we're in an auto mode - Device starts with a 20-second delay before deciding on what to do
    // during this time we await any commands
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
}

extern "C" void app_main(void)
{
    dependencyRegistry->registerService<ProjectConfig>(DependencyType::project_config, deviceConfig);
    dependencyRegistry->registerService<CameraManager>(DependencyType::camera_manager, cameraHandler);
    dependencyRegistry->registerService<WiFiManager>(DependencyType::wifi_manager, wifiManager);
    dependencyRegistry->registerService<LEDManager>(DependencyType::led_manager, ledManager);

    // add endpoint to check firmware version
    // add firmware version somewhere
    // setup CI and building for other boards
    // finish todos, overhaul stuff a bit

    // todo - do we need logs over CDC? Or just commands and their results?
    // esp_log_set_vprintf(&websocket_logger);
    Logo::printASCII();
    initNVSStorage();
    deviceConfig->load();
    ledManager->setup();

    xTaskCreate(
        HandleStateManagerTask,
        "HandleStateManagerTask",
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

    cameraHandler->setupCamera();

    // let's keep the serial manager running for the duration of the setup
    // we'll clean it up later if need be
    serialManager->setup();
    xTaskCreate(
        HandleSerialManagerTask,
        "HandleSerialManagerTask",
        1024 * 6,
        serialManager,
        1,
        &serialManagerHandle);

    StreamingMode mode = deviceConfig->getDeviceMode();
    if (mode == StreamingMode::UVC)
    {
        // in UVC mode we only need to start the bare essentials for UVC
        // we don't need any wireless communication, we can shut it down

        // todo this would be the perfect place to introduce random delays
        // to workaround windows usb bug
        startWiredMode(true);
    }
    else if (mode == StreamingMode::WIFI)
    {
        // in Wifi mode we only need the wireless communication stuff, nothing else got started
        startWiFiMode(true);
    }
    else
    {
        // since we're in setup mode, we have to have wireless functionality on,
        // so we can do wifi scanning, test connection etc
        startWiFiMode(false);
        startSetupMode();
    }
}