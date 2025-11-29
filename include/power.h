#include <Arduino.h>

#ifndef __POWER__H__
typedef struct {
    int32_t bat_level;
    int32_t bat_current;
    bool ext_power;
    bool charging_enabled;
    bool is_charging;
} BatteryInfo;
#define __POWER__H__

BatteryInfo get_power();

void display_power_ui();

#endif