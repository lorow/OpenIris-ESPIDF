#include "main_globals.hpp"
#include "esp_log.h"

// Forward declarations
extern void start_video_streaming(void *arg);

// Global variables to be set by main
static esp_timer_handle_t* g_streaming_timer_handle = nullptr;
static TaskHandle_t* g_serial_manager_handle = nullptr;

// Functions for main to set the global handles
void setStreamingTimerHandle(esp_timer_handle_t* handle) {
    g_streaming_timer_handle = handle;
}

void setSerialManagerHandle(TaskHandle_t* handle) {
    g_serial_manager_handle = handle;
}

// Functions for components to access the handles
esp_timer_handle_t* getStreamingTimerHandle() {
    return g_streaming_timer_handle;
}

TaskHandle_t* getSerialManagerHandle() {
    return g_serial_manager_handle;
}

// Global pause state
bool startupPaused = false;

// Function to manually activate streaming
void activateStreaming(bool disableSetup) {
    ESP_LOGI("[MAIN_GLOBALS]", "Manually activating streaming, disableSetup=%s", disableSetup ? "true" : "false");
    
    TaskHandle_t* serialHandle = disableSetup ? g_serial_manager_handle : nullptr;
    void* serialTaskHandle = (serialHandle && *serialHandle) ? *serialHandle : nullptr;
    
    start_video_streaming(serialTaskHandle);
}