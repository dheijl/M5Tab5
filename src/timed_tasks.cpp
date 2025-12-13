#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>
#include "timed_tasks.h"
#include "power.h"
#include "wifi_connect.h"
#include "ui.h"

static void log_ram()
{
    log_d("Total heap: %d", ESP.getHeapSize());
    log_d("Free heap: %d", ESP.getFreeHeap());
    log_d("Total PSRAM: %d", ESP.getPsramSize());
    log_d("Free PSRAM: %d", ESP.getFreePsram());
}



/// @brief CheckPowerOff: check every 5 minutes if poweroff desired
/// @param tasktimers 
void CheckPowerOff(void* params)
{
    while (1) {
        vTaskDelay(5 * 60 * 1000);
        MPD_PLAYER* mpd_pl = (MPD_PLAYER*)params;
        WiFi.setSleep(false);
        auto bi = get_power();
        // battery low and not charging
        if (bi.bat_level < 20 && !bi.is_charging) {
            WiFi.disconnect();
            M5.Power.powerOff();
        }
        MPD_Client mpd(*mpd_pl);
        // not playing and not on external power
        if (!mpd.is_playing() && (bi.bat_current > 10)) {
            WiFi.disconnect();
            M5.Power.powerOff();
        }
        else {
            WiFi.setSleep(true);
        }
    }
}


/// @brief check if touchscreen pressed
void CheckTouch(void* params)
{
    static P1_DATA dongle_data;
    uint32_t display_timer = 0;
    uint32_t minute_timer = 0;
    m5::rtc_datetime_t now = M5.Rtc.getDateTime();
    uint8_t second = now.time.seconds;
    uint8_t minute = now.time.minutes;
    bool sleeping = true;
    while (1) {
        vTaskDelay(100);
        now = M5.Rtc.getDateTime();

        M5.update();
        auto touch = M5.Touch.getDetail();
        if (touch.wasPressed()) {
            sleeping = false;
            display_timer = 0;
            // update power display (values without wifi and display)
            display_power_ui();
            // prepare WiFi and display 
            WiFi.setSleep(false);
            M5.Display.powerSaveOff();
            M5.Display.setBrightness(40);
            if (WiFi.status() != WL_CONNECTED) {
                connect_wifi();
            }
            // show mpd status
            MPD_PLAYER* mpd_pl = (MPD_PLAYER*)params;
            MPD_Client mpd(*mpd_pl);
            display_mpd_ui(&mpd);
            // show P1 meter dongle data 
            if (p1_request(dongle_data)) {
                display_dongle_ui(dongle_data);
            }
            WiFi.setSleep(true);
            log_ram();
        }
        if (now.time.seconds != second) {
            second = now.time.seconds;
            display_timer++;
        }
        if (display_timer >= 8) {
            display_timer = 0;
            sleeping = true;
            M5.Display.setBrightness(0);
            WiFi.setSleep(true);
            M5.Display.powerSaveOn();
        }
        if (!sleeping) {
            if (now.time.minutes != minute) {
                minute = now.time.minutes;
                display_date_time_ui();
            }
        }
    }
}
