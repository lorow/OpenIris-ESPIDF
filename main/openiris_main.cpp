#include <stdio.h>
#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_camera.h"
#include "nvs_flash.h"
#include "esp_psram.h"

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
#include <RestAPI.hpp>

#include <stdarg.h>

#ifdef CONFIG_WIRED_MODE
#include <UVCStream.hpp>
#endif

#define BLINK_GPIO (gpio_num_t) CONFIG_BLINK_GPIO
#define CONFIG_LED_C_PIN_GPIO (gpio_num_t) CONFIG_LED_C_PIN

static const char *TAG = "[MAIN]";

QueueHandle_t eventQueue = xQueueCreate(10, sizeof(SystemEvent));
QueueHandle_t ledStateQueue = xQueueCreate(10, sizeof(uint32_t));

StateManager *stateManager = new StateManager(eventQueue, ledStateQueue);
std::shared_ptr<DependencyRegistry> dependencyRegistry = std::make_shared<DependencyRegistry>();
std::shared_ptr<CommandManager> commandManager = std::make_shared<CommandManager>(dependencyRegistry);

WebSocketLogger webSocketLogger;
Preferences preferences;

std::shared_ptr<ProjectConfig> deviceConfig = std::make_shared<ProjectConfig>(&preferences);
WiFiManager wifiManager(deviceConfig, eventQueue, stateManager);
MDNSManager mdnsManager(deviceConfig, eventQueue);

std::shared_ptr<CameraManager> cameraHandler = std::make_shared<CameraManager>(deviceConfig, eventQueue);
StreamServer streamServer(80, stateManager);

RestAPI *restAPI = new RestAPI("http://0.0.0.0:81", commandManager);

#ifdef CONFIG_WIRED_MODE
UVCStreamManager uvcStream;
#endif

LEDManager *ledManager = new LEDManager(BLINK_GPIO, CONFIG_LED_C_PIN_GPIO, ledStateQueue);

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
    // with that, I couuld use vtaskdelayuntil to advance and display states
    // and with that, I should rethink how state management works

    // rethink state management - DONE

    // port serial manager
    // add support of commands to UVC
    // add endpoint to check firmware version
    // add firmware version somewhere
    // setup CI and building for other boards
    // finish todos, overhaul stuff a bit

    Logo::printASCII();
    initNVSStorage();

    // esp_log_set_vprintf(&test_log);
    ledManager->setup();

    xTaskCreate(
        HandleStateManagerTask,
        "HandleLEDDisplayTask",
        1024 * 2,
        stateManager,
        3,
        NULL // it's fine for us not get a handle back, we don't need it
    );

    xTaskCreate(
        HandleLEDDisplayTask,
        "HandleLEDDisplayTask",
        1024 * 2,
        ledManager,
        3,
        NULL);

    deviceConfig->load();
    wifiManager.Begin();
    mdnsManager.start();
    restAPI->begin();
    cameraHandler->setupCamera();
    // make sure the server runs on a separate core
    streamServer.startStreamServer();

    xTaskCreate(
        HandleRestAPIPollTask,
        "HandleRestAPIPollTask",
        1024 * 2,
        restAPI,
        1, // it's the rest API, we only serve commands over it so we don't really need a higher priority
        NULL);

#ifdef CONFIG_WIRED_MODE
    uvcStream.setup();
#endif
}
