#include <M5Unified.h>
#include <M5GFX.h>

#include "power.h"

BatteryInfo get_power() {
    BatteryInfo bat_info = { 0, 0, false, false, false };
    // M5.Power.setChargeCurrent(500); // not implemented on TAB5
    bat_info.bat_level = M5.Power.getBatteryLevel();
    bat_info.bat_current = M5.Power.getBatteryCurrent();
    if (bat_info.bat_level <= 70) {
        //log_d("enable battery charging");
        M5.Power.setBatteryCharge(true);
        bat_info.charging_enabled = true;
    }
    else if (bat_info.bat_level >= 85) {
        //log_d("disable battery charging");
        M5.Power.setBatteryCharge(false);
        bat_info.charging_enabled = false;
    }
    vTaskDelay(100);
    switch (M5.Power.isCharging()) {
    case M5.Power.is_discharging:
        bat_info.is_charging = false;
        bat_info.ext_power = false;
        break;
    case M5.Power.is_charging:
        bat_info.is_charging = true;
        bat_info.ext_power = true;
    default:
        break;
    }
    return bat_info;
}

