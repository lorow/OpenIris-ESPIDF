#include "main_globals.hpp"
#include "esp_log.h"

// Forward declarations
extern void start_video_streaming(void *arg);

bool startupCommandReceived = false;
bool getStartupCommandReceived()
{
    return startupCommandReceived;
}

void setStartupCommandReceived(bool startupCommandReceived)
{
    startupCommandReceived = startupCommandReceived;
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
bool startupPaused = false;
bool getStartupPaused()
{
    return startupPaused;
}

void setStartupPaused(bool startupPaused)
{
    startupPaused = startupPaused;
}

// Function to manually activate streaming
void activateStreaming(bool disableSetup)
{
    ESP_LOGI("[MAIN_GLOBALS]", "Manually activating streaming, disableSetup=%s", disableSetup ? "true" : "false");

    TaskHandle_t *serialHandle = disableSetup ? g_serial_manager_handle : nullptr;
    void *serialTaskHandle = (serialHandle && *serialHandle) ? *serialHandle : nullptr;

    start_video_streaming(serialTaskHandle);
}