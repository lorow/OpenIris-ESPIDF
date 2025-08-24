#include "main_globals.hpp"
#include "esp_log.h"

// Forward declarations
extern void start_video_streaming(void *arg);

static bool s_startupCommandReceived = false;
bool getStartupCommandReceived()
{
    return s_startupCommandReceived;
}

void setStartupCommandReceived(bool startupCommandReceived)
{
    s_startupCommandReceived = startupCommandReceived;
}

static TaskHandle_t *g_serial_manager_handle = nullptr;
TaskHandle_t *getSerialManagerHandle()
{
    return g_serial_manager_handle;
}

void setSerialManagerHandle(TaskHandle_t *serialManagerHandle)
{
    g_serial_manager_handle = serialManagerHandle;
}

// Global pause state
static bool s_startupPaused = false;
bool getStartupPaused()
{
    return s_startupPaused;
}

void setStartupPaused(bool startupPaused)
{
    s_startupPaused = startupPaused;
}

// Function to manually activate streaming
void activateStreaming(bool disableSetup)
{
    ESP_LOGI("[MAIN_GLOBALS]", "Manually activating streaming, disableSetup=%s", disableSetup ? "true" : "false");

    TaskHandle_t *serialHandle = disableSetup ? g_serial_manager_handle : nullptr;
    void *serialTaskHandle = (serialHandle && *serialHandle) ? *serialHandle : nullptr;

    start_video_streaming(serialTaskHandle);
}

// USB handover state
static bool s_usbHandoverDone = false;
bool getUsbHandoverDone() { return s_usbHandoverDone; }
void setUsbHandoverDone(bool done) { s_usbHandoverDone = done; }