
// for monitor: sudo modprobe ch341 and/or cp210x

#include <Arduino.h>
#include <M5GFX.h>
#include <M5Unified.h>

#include "wifi_connect.h"
#include "power.h"
#include "synctime.h"
#include "ui.h"
#include "mpdcli.h"
#include "sdcard_fs.h"

#include "p1dongle.h"

static SD_Config sd_card;
static m5::rtc_datetime_t now;

static MPD_PLAYER mpd_pl;
static P1_DATA dongle_data;

typedef struct task_timers {
    bool sec_elapsed;
    bool min_elapsed;
    bool five_min_elapsed;
    bool display_timer_elapsed;
} TASK_TIMERS;

const uint32_t DISPLAY_TIME_SECS = 7; // seconds display stays on after touch

static TASK_TIMERS UpdateTimers(bool sleeping);
static bool CheckTouch();
static bool UpdateStatus(const TASK_TIMERS& tasktimers);
static void CheckPowerOff();
static void ShowData();
static void log_ram();

void setup()
{
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Power.setExtOutput(false, m5::ext_port_mask_t(255));
    WiFi.setPins(SDIO2_CLK, SDIO2_CMD, SDIO2_D0, SDIO2_D1, SDIO2_D2, SDIO2_D3, SDIO2_RST);

    Serial.begin(115200);
    // display 
    M5.Display.setRotation(0);
    M5.Display.setTextDatum(top_left);
    M5.Display.fillScreen(DARKGREY);
    M5.Display.setTextColor(GREENYELLOW);
    M5.Display.setFont(&fonts::FreeMonoBold12pt7b);
    M5.Display.setTextScroll(true);
    M5.Display.waitDisplay();
    M5.Display.setBrightness(10);

    // display battery/power state on top line 
    display_power_ui();

    // check battery low
    M5.Display.setCursor(0, 64);
    auto bat_info = get_power();
    if ((bat_info.bat_level < 20) && !(bat_info.ext_power | bat_info.is_charging)) {
        M5.Display.println("Battery low - please connect external power.");
        vTaskDelay(2000);
        M5.Power.powerOff();
    }

    // initialize NVS network config from SD if SD present
    NETWORK_CFG nw_cfg;
    if (SD_Config::read_wifi(nw_cfg)) {
        log_d("saving wifi to NVS: %s %s", nw_cfg.ssid, nw_cfg.psw);
        if (!write_wifi(nw_cfg)) {
            log_d("error writing NVS wifi!");
        }
    }
    // start wifi connection and show on topline 
    connect_wifi();
    check_time();

    // display datetime on second line, using SNTP sync once a day
    display_date_time_ui();

    // initialize NVS player config from SD if SD present 
    // needs WIFI for MDNS lookup
    MPD_PLAYER player;
    if (SD_Config::read_player(player)) {
        log_d("saving player to NVS: %s %s %s %d", player.player_name, player.player_hostname, player.player_ip, player.player_port);
        if (!write_player(player)) {
            log_d("error writing NVS player!");
        }
    }

    // connect to MPD player and show status
    if (!read_player(mpd_pl)) {
        mpd_pl.player_name = "NVS error";
        mpd_pl.player_hostname = "NVS error";
        mpd_pl.player_ip = "0.0.0.0";
        mpd_pl.player_port = 0;
    }
    else {
        ShowData();
    }

    M5.Display.setCursor(0, 64);

}


void loop()
{
    static bool sleeping = false;

    // update timers
    TASK_TIMERS tasktimers = UpdateTimers(sleeping);
    // check touchscreen
    if (CheckTouch()) {
        sleeping = false;
        ShowData();
    }
    // if touched: update time/power status while not sleeping
    if (!sleeping) {
        sleeping = UpdateStatus(tasktimers);
    }
    // every 5 mins: check if on battery and not playing => poweroff
    if (tasktimers.five_min_elapsed) CheckPowerOff();
    // and continue
    vTaskDelay(100);
}

/// @brief CheckPowerOff: check every 5 minutes if poweroff desired
static void CheckPowerOff()
{
    auto bi = get_power();
    // battery low and not charging
    if (bi.bat_level < 20 && !bi.is_charging) {
        WiFi.disconnect();
        M5.Power.powerOff();
    }
    WiFi.setSleep(false);
    MPD_Client mpd(mpd_pl);
    // not playing and not on external power
    if (!mpd.is_playing() && (bi.bat_current > 10)) {
        WiFi.disconnect();
        M5.Power.powerOff();
    }
    else {
        WiFi.setSleep(true);
    }
}

/// @brief check if touchscreen pressed
static bool CheckTouch()
{
    bool touched = false;
    M5.update();
    auto touch = M5.Touch.getDetail();
    if (touch.wasPressed()) {
        touched = true;
    }
    return touched;
}

/// @brief show MPD and P1-Dongle data
static void ShowData() {
    // update power display (values without wifi and display)
    display_power_ui();
    // prepare WiFi and display 
    WiFi.setSleep(false);
    M5.Display.powerSaveOff();
    M5.Display.setBrightness(10);
    if (WiFi.status() != WL_CONNECTED) {
        connect_wifi();
    }
    // show mpd status
    MPD_Client mpd(mpd_pl);
    display_mpd_ui(&mpd);
    // show P1 meter dongle data 
    if (p1_request(dongle_data)) {
        display_dongle_ui(dongle_data);
    }
    WiFi.setSleep(true);
}

/// @brief the display is showing: update as necessary and enter sleep mode if necessary
/// @param tasktimers 
/// @returns bool: enter sleep or not yet
static bool UpdateStatus(const TASK_TIMERS& tasktimers)
{

    if (tasktimers.sec_elapsed) {
        display_date_time_ui();
    }
    if (tasktimers.min_elapsed) {
        display_power_ui();
    }
    if (tasktimers.display_timer_elapsed) {
        M5.Display.setBrightness(0);
        WiFi.setSleep(true);
        M5.Display.powerSaveOn();
        log_ram();
        return true;
    }
    return false;
}

/// @brief update the timers that say what to do
/// @param tasktimers 
static TASK_TIMERS UpdateTimers(bool sleeping)
{
    static int8_t last_second = 0;
    static int8_t last_minute = 0;
    static uint32_t minutes_elapsed = 0;
    static uint32_t display_timer = 0;

    TASK_TIMERS tasktimers = { false, false, false, false };

    now = M5.Rtc.getDateTime();
    if (now.time.seconds != last_second) {
        tasktimers.sec_elapsed = true;
        last_second = now.time.seconds;
        if (!sleeping) {
            display_timer++;
            if (display_timer >= DISPLAY_TIME_SECS) {
                tasktimers.display_timer_elapsed = true;
                display_timer = 0;
            }
        }
    }
    if (now.time.minutes != last_minute) {
        minutes_elapsed++;
        last_minute = now.time.minutes;
        tasktimers.min_elapsed = true;
        if (minutes_elapsed == 5) {
            minutes_elapsed = 0;
            tasktimers.five_min_elapsed = true;
        }
    }
    return tasktimers;
}

/// @brief show relevant RAM/PSRAM size
static void log_ram()
{
    log_d("Total heap: %d", ESP.getHeapSize());
    log_d("Free heap: %d", ESP.getFreeHeap());
    log_d("Total PSRAM: %d", ESP.getPsramSize());
    log_d("Free PSRAM: %d", ESP.getFreePsram());
}
