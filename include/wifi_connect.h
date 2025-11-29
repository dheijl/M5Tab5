#ifndef __CONNECT__WIFI__H__
#define __CONNECT__WIFI__H__

#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <esp_wifi.h>

#define SDIO2_CLK GPIO_NUM_12
#define SDIO2_CMD GPIO_NUM_13
#define SDIO2_D0  GPIO_NUM_11
#define SDIO2_D1  GPIO_NUM_10
#define SDIO2_D2  GPIO_NUM_9
#define SDIO2_D3  GPIO_NUM_8
#define SDIO2_RST GPIO_NUM_15

#include "nvs_config.h"

void connect_wifi();
void wifi_scan();

#endif
