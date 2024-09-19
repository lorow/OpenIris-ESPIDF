/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "usb_device_uvc.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include <openiris_logo.hpp>
#include <wifiManager.hpp>
#include <ProjectConfig.hpp>

static const char *TAG = "[MAIN]";

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO (gpio_num_t) CONFIG_BLINK_GPIO

static uint8_t s_led_state = 0;

static void blink_led(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLINK_GPIO, s_led_state);
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

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
    // then port the led manager as this will be fairly easy
    // then port the serial manager
    // then port the camera manager
    // then port the streaming stuff (web and uvc)
    // then port the async web server
    // then port the Elegant OTA stuff
    // then port the mdns stuff

    // TODO add this option
    // ProjectConfig deviceConfig("openiris", MDNS_HOSTNAME);
    ProjectConfig deviceConfig("openiris", "openiristracker");
    WiFiManager wifiManager;

    Logo::printASCII();
    initNVSStorage();

    deviceConfig.load();
    wifiManager.Begin();

    /* Configure the peripheral according to the LED type */
    configure_led();

    while (1)
    {
        ESP_LOGI(TAG, "Turning the LED on pin %d %s!", BLINK_GPIO, s_led_state == true ? "ON" : "OFF");
        blink_led();
        /* Toggle the LED state */
        s_led_state = !s_led_state;
        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
    }
}
