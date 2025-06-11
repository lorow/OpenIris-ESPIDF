#pragma once
#ifndef MAIN_GLOBALS_HPP
#define MAIN_GLOBALS_HPP

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Functions for main to set global handles
void setStreamingTimerHandle(esp_timer_handle_t* handle);
void setSerialManagerHandle(TaskHandle_t* handle);

// Functions to access global handles from components
esp_timer_handle_t* getStreamingTimerHandle();
TaskHandle_t* getSerialManagerHandle();

// Function to manually activate streaming
void activateStreaming(bool disableSetup = false);

// Function to notify that a command was received during startup
extern void notify_startup_command_received();

// Global variables for startup state
extern bool startupCommandReceived;
extern esp_timer_handle_t startupTimerHandle;
extern bool startupPaused;

#endif