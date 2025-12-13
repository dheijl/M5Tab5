
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

#include "timed_tasks.h"

#include "p1dongle.h"

static SD_Config sd_card;
static m5::rtc_datetime_t now;

static MPD_PLAYER mpd_pl;
static P1_DATA dongle_data;


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
    M5.Display.setBrightness(40);

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
        MPD_Client mpd(mpd_pl);
        display_mpd_ui(&mpd);
    }
    // show current NRG P1 dongle data
    if (p1_request(dongle_data)) {
        display_dongle_ui(dongle_data);
    }

    M5.Display.setCursor(0, 64);

    TaskHandle_t xHandle_pwr = NULL;
    xTaskCreate(CheckPowerOff, "CHKPOWER", 4096, &mpd_pl, 2, &xHandle_pwr);
    TaskHandle_t xHandle_touch = NULL;
    xTaskCreate(CheckTouch, "CHKTOUCH", 16384, &mpd_pl, 2, &xHandle_touch);

}



void loop()
{
    vTaskDelay(10000);
}

