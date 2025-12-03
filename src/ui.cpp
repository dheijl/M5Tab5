#include "ui.h"

// power/charging display 0,0 .. 360,32 (wifi ui is 360,0 .. 720,32)
void display_power_ui() {
    auto bat_info = get_power();
    int32_t wf = (60 * bat_info.bat_level) / 100;
    int32_t we = 60 - wf;
    M5Canvas pwr_canvas(&M5.Display);
    pwr_canvas.setPsram(true);
    pwr_canvas.createSprite(360, 32);
    pwr_canvas.setFont(&fonts::FreeMonoBold12pt7b);
    pwr_canvas.setTextDatum(top_left);
    pwr_canvas.setTextColor(WHITE);
    // battery icon
    int bat_color = bat_info.bat_level >= 40 ? GREEN : RED;
    pwr_canvas.fillRoundRect(2, 2, wf, 26, 4, bat_color);
    pwr_canvas.fillRoundRect(wf + 1, 2, we - 1, 26, 4, WHITE);
    // battery level
    pwr_canvas.drawString(std::to_string(bat_info.bat_level).c_str(), 80, 4);
    pwr_canvas.drawString("%", 110, 4);
    // battery current
    pwr_canvas.setTextColor(bat_info.bat_current >= 0 ? WHITE : GREEN);
    pwr_canvas.drawString((std::to_string(bat_info.bat_current) + std::string(" mAh")).c_str(), 140, 4);
    // external power
    if (bat_info.ext_power) {
        pwr_canvas.setTextColor(bat_info.is_charging ? GREEN : WHITE);
        pwr_canvas.drawString("EXTPWR", 260, 4);
    }
    pwr_canvas.pushSprite(0, 0);
    pwr_canvas.deleteSprite(); // not really needed
}

// date-time display 0,32 .. 360,64
void display_date_time_ui() {
    M5Canvas pwr_canvas(&M5.Display);
    pwr_canvas.setPsram(true);
    pwr_canvas.createSprite(360, 32);
    pwr_canvas.setFont(&fonts::FreeMonoBold12pt7b);
    pwr_canvas.setTextDatum(top_left);
    pwr_canvas.setTextColor(WHITE);
    pwr_canvas.setCursor(0, 4);
    m5::rtc_date_t d;
    m5::rtc_time_t t;
    bool rc = (M5.Rtc.getDate(&d) & M5.Rtc.getTime(&t));
    if (rc) {
        pwr_canvas.printf("%04d/%02d/%02d - %02d:%02d:%02d", d.year, d.month, d.date, t.hours, t.minutes, t.seconds);
    }
    else {
        sync_time();
        bool rc = (M5.Rtc.getDate(&d) & M5.Rtc.getTime(&t));
        pwr_canvas.printf("SNTP: %04d/%02d/%02d - %02d:%02d:%02d", d.year, d.month, d.date, t.hours, t.minutes, t.seconds);
    }
    pwr_canvas.pushSprite(0, 32);
    pwr_canvas.deleteSprite();

}

// mpd ui: 360,32 .. 720,64 and 0,64 .. 720,364
void display_mpd_ui(MPD_Client* mpd_cli) {
    // mpd status
    M5Canvas mpd_canvas(&M5.Display);
    mpd_canvas.setPsram(true);
    mpd_canvas.createSprite(360, 32);
    mpd_canvas.setFont(&fonts::FreeMonoBold12pt7b);
    mpd_canvas.setTextDatum(top_left);
    mpd_canvas.setCursor(0, 4);
    auto player = mpd_cli->get_player();
    auto playing = mpd_cli->is_playing();
    if (playing) {
        mpd_canvas.setTextColor(GREEN);
    }
    else {
        mpd_canvas.setTextColor(LIGHTGREY);
    }
    mpd_canvas.printf("%s@%s", player.player_name, player.player_ip);
    mpd_canvas.pushSprite(360, 32);
    mpd_canvas.deleteSprite();
    // now playing info
    M5Canvas pl_canvas(&M5.Display);
    pl_canvas.setPsram(true);
    pl_canvas.createSprite(720, 300);
    pl_canvas.setFont(&fonts::Font4);
    pl_canvas.setTextSize(1.2);
    //pl_canvas.setFont(&fonts::FreeSans12pt7b);
    pl_canvas.setTextDatum(top_left);
    pl_canvas.setTextWrap(false, false);
    pl_canvas.setBaseColor(BG_COLOR);
    pl_canvas.fillScreen(BG_COLOR);
    if (playing) {
        pl_canvas.setTextColor(ORANGE);
    }
    else {
        pl_canvas.setTextColor(DARKGREY);
    }
    int32_t x = 0;
    int32_t y = 4;
    auto sl = mpd_cli->show_mpd_status();
    for (auto l : sl) {
        pl_canvas.setCursor(x, y);
        y += 34;
        pl_canvas.print(l);
    }
    pl_canvas.pushSprite(0, 64);
    pl_canvas.deleteSprite();
}

// P1 dongle ui text: 0,364 .. 720,764
// P1 dongle graph  : 0,764 .. 720,1164
void display_dongle_ui(const P1_DATA dongle_data)
{
    M5Canvas p1_canvas(&M5.Display);
    p1_canvas.setPsram(true);
    p1_canvas.createSprite(720, 400);
    p1_canvas.setFont(&fonts::Font4);
    p1_canvas.setTextSize(1.2);
    //pl_canvas.setFont(&fonts::FreeSans12pt7b);
    p1_canvas.setTextDatum(top_left);
    p1_canvas.setTextWrap(false, false);
    p1_canvas.setBaseColor(P1_BG_COLOR);
    p1_canvas.fillScreen(P1_BG_COLOR);
    int32_t x = 0;
    int32_t y = 10;
    p1_canvas.setCursor(x, y);
    p1_canvas.setTextColor(GREEN);
    p1_canvas.print("Smart Meter data:");
    y += 34; p1_canvas.setCursor(x, y);
    p1_canvas.setTextColor(ORANGE);
    p1_canvas.printf("KWh peak: %.3f", dongle_data.highest_peak_pwr);
    y += 34; p1_canvas.setCursor(x, y);
    p1_canvas.printf("KWh <-: %.3f", dongle_data.power_delivered);
    y += 34; p1_canvas.setCursor(x, y);
    p1_canvas.printf("KWh ->: %.3f", dongle_data.power_returned);
    y += 34; p1_canvas.setCursor(x, y);
    p1_canvas.printf("A l1: %.3f", dongle_data.current_l1);
    y += 34; p1_canvas.setCursor(x, y);
    p1_canvas.printf("A l3: %.3f", dongle_data.current_l3);
    y += 34; p1_canvas.setCursor(x, y);
    p1_canvas.printf("KWh used T1: %.3f", dongle_data.energy_delivered_tariff1);
    y += 34; p1_canvas.setCursor(x, y);
    p1_canvas.printf("KWh used T2: %.3f", dongle_data.energy_delivered_tariff2);
    y += 34; p1_canvas.setCursor(x, y);
    p1_canvas.printf("KWh returned T1: %.3f", dongle_data.energy_returned_tariff1);
    y += 34; p1_canvas.setCursor(x, y);
    p1_canvas.printf("KWh returned T2: %.3f", dongle_data.energy_returned_tariff2);
    y += 34; p1_canvas.setCursor(x, y);
    p1_canvas.printf("m3 gas used: %.3f", dongle_data.gas_delivered);
    p1_canvas.pushSprite(0, 364);
    p1_canvas.deleteSprite();

    M5Canvas ch_canvas(&M5.Display);
    ch_canvas.setPsram(true);
    ch_canvas.createSprite(720, 400);
    ch_canvas.setFont(&fonts::Font4);
    ch_canvas.setTextSize(1.2);
    //pl_canvas.setFont(&fonts::FreeSans12pt7b);
    ch_canvas.setTextDatum(top_left);
    ch_canvas.setTextWrap(false, false);
    ch_canvas.setBaseColor(CH_BG_COLOR);
    ch_canvas.fillScreen(CH_BG_COLOR);

    float total_KWh_in = dongle_data.energy_delivered_tariff1 + dongle_data.energy_delivered_tariff2;
    float total_KWh_out = dongle_data.energy_returned_tariff1 + dongle_data.energy_returned_tariff2;
    ch_canvas.setColor(BLUE);
    int h_KWH_in = (int)trunc(16.5 / total_KWh_in);
    log_d("KWh_in: %d", h_KWH_in);
    ch_canvas.fillRect(5, 400, 25, h_KWH_in);

    ch_canvas.pushSprite(0, 764);
    ch_canvas.deleteSprite();
}
