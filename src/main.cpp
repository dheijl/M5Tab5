
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
static uint32_t seconds = 0;
static uint32_t minutes = 0;
static uint32_t minutes_elapsed = 0;
static m5::rtc_datetime_t now;
static uint32_t screen_timer = 0;
static bool sleeping = false;
static bool five_min_elapsed = false;

static MPD_PLAYER mpd_pl;
static P1_DATA dongle_data;

static void log_ram()
{
    log_d("Total heap: %d", ESP.getHeapSize());
    log_d("Free heap: %d", ESP.getFreeHeap());
    log_d("Total PSRAM: %d", ESP.getPsramSize());
    log_d("Free PSRAM: %d", ESP.getFreePsram());
}

void setup()
{
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Power.setExtOutput(false, m5::ext_port_mask_t(255));
    WiFi.setPins(SDIO2_CLK, SDIO2_CMD, SDIO2_D0, SDIO2_D1, SDIO2_D2, SDIO2_D3, SDIO2_RST);

    esp_pm_config_t config = { 360, 40, true };
    esp_pm_configure(&config);


    Serial.begin(115200);
    // display 
    M5.Display.setRotation(0);
    M5.Display.setTextDatum(top_left);
    M5.Display.fillScreen(DARKGREY);
    M5.Display.setTextColor(GREENYELLOW);
    M5.Display.setFont(&fonts::FreeMonoBold12pt7b);
    M5.Display.setTextScroll(true);
    M5.Display.waitDisplay();
    M5.Display.setBrightness(40);
    display_power_ui();

    // initialize network from SD if SD present
    NETWORK_CFG nw_cfg;
    if (SD_Config::read_wifi(nw_cfg)) {
        log_d("saving wifi to NVS: %s %s", nw_cfg.ssid, nw_cfg.psw);
        if (! write_wifi(nw_cfg)) {
            log_d("error writing NVS wifi!");
        }
    }

    connect_wifi();
    check_time();

    display_date_time_ui();

    // initialize player config from SD if SD present
    // needs WIFI for MDNS lookup
    MPD_PLAYER player;
    if (SD_Config::read_player(player)) {
        log_d("saving player to NVS: %s %s %s %d", player.player_name, player.player_hostname, player.player_ip, player.player_port);
        if (!write_player(player)) {
            log_d("error writing NVS player!");
        }
    }


    M5.Display.setCursor(0, 64);
    auto bat_info = get_power();
    if ((bat_info.bat_level < 20) && !(bat_info.ext_power | bat_info.is_charging)) {
        M5.Display.println("Battery low - please connect external power.");
        vTaskDelay(2000);
        M5.Power.powerOff();
    }
    if (! read_player(mpd_pl)) {
        mpd_pl.player_name = "NVS error";
        mpd_pl.player_hostname = "NVS error";
        mpd_pl.player_ip = "0.0.0.0";
        mpd_pl.player_port = 0;
    }
    MPD_Client mpd(mpd_pl);
    display_mpd_ui(&mpd);
    // show current NRG P1 dongle data
    if (p1_request(dongle_data)) {
        display_dongle_ui(dongle_data);
    }

}

const uint32_t DISPLAY_TIME = 7; // seconds display stays on after touch

void loop()
{
    bool sec_elapsed = false;
    bool min_elapsed = false;
    now = M5.Rtc.getDateTime();
    if (now.time.seconds != seconds) {
        sec_elapsed = true;
        screen_timer++;
        seconds = now.time.seconds;
        if (now.time.minutes != minutes) {
            minutes = now.time.minutes;
            min_elapsed = true;
            minutes_elapsed++;
            if (minutes_elapsed == 5) {
                five_min_elapsed = true;
                minutes_elapsed = 0;
            }
        }
    }
    if (!sleeping) {
        M5.Display.setCursor(0, 64);
        if (min_elapsed) {
            display_power_ui();
        }
        if (sec_elapsed) {
            display_date_time_ui();
            if (!WiFi.isConnected()) {
                connect_wifi();
            }
        }
        if (screen_timer >= DISPLAY_TIME) { // 7 seconds 
            screen_timer = 0;
            M5.Display.setBrightness(0);
            WiFi.setSleep(true);
            M5.Display.powerSaveOn();
            sleeping = true;
        }
    } 
    // check after 5 mins if not on external power and not playing
    if (five_min_elapsed) {
        five_min_elapsed = false;
        WiFi.setSleep(false);
        auto bi = get_power();
        // battery low and not charging
        if (bi.bat_level < 20 && !bi.is_charging) {
            WiFi.disconnect();
            M5.Power.powerOff();
        }
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
    // check touch, sleeping or not    
    M5.update();
    auto touch = M5.Touch.getDetail();
    if (touch.wasPressed()) {
        screen_timer = 0;
        sleeping = false;
        display_power_ui(); // show before wifi and screen !!
        WiFi.setSleep(false);
        M5.Display.setBrightness(40);
        M5.Display.powerSaveOff();
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
        log_ram();
    }
    vTaskDelay(50);
}
 