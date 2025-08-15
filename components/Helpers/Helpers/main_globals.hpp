#pragma once
#ifndef MAIN_GLOBALS_HPP
#define MAIN_GLOBALS_HPP

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Functions for main to set global handles

// Functions to access global handles from components
TaskHandle_t *getSerialManagerHandle();
void setSerialManagerHandle(TaskHandle_t *serialManagerHandle);

// Function to manually activate streaming
void activateStreaming(bool disableSetup = false);

bool getStartupCommandReceived();
void setStartupCommandReceived(bool startupCommandReceived);

bool getStartupPaused();
void setStartupPaused(bool startupPaused);

#endif