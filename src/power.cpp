#include <M5Unified.h>
#include <M5GFX.h>

#include "power.h"

// 2S LiPo: anything below 6V is either no battery or a dead one.
// Without a battery the INA226 bus voltage floats a few mV above 0
// (LSB = 1.25mV), so a near-zero threshold gives false "battery present".
const int16_t MIN_BATTERY_MV = 6000;

BatteryInfo get_power() {
    BatteryInfo bat_info = { false, 0, 0, false, false, false };
    if (M5.Power.getBatteryVoltage() < MIN_BATTERY_MV) {
        return bat_info;
    }
    bat_info.bat_present = true;
    // M5.Power.setChargeCurrent(500); // not implemented on TAB5
    bat_info.bat_level = M5.Power.getBatteryLevel();
    bat_info.bat_current = M5.Power.getBatteryCurrent();
    if (bat_info.bat_level <= 50) {
        //log_d("enable battery charging");
        M5.Power.setBatteryCharge(true);
        bat_info.charging_enabled = true;
    }
    else if (bat_info.bat_level >= 95) {
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

