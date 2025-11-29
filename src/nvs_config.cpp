#include <Arduino.h>

#include "nvs_config.h"

bool read_wifi(NETWORK_CFG& nw_cfg)
{
    Preferences prefs;
    if (!prefs.begin(NVS_WIFI, true)) {
        log_d("wifi prefs begin error");
        prefs.end();
        return false;
    }
    String ssid = prefs.getString("ssid");
    String psw = prefs.getString("psw");
    prefs.end();
    log_d("%s %s", ssid.c_str(), psw.c_str());
    if (ssid.isEmpty() || psw.isEmpty()) {
        log_d("empty wifi prefs!");
        return false;
    }
    nw_cfg.ssid = strdup(ssid.c_str());
    nw_cfg.psw = strdup(psw.c_str());
    log_d("Wifi config: %s - %s", nw_cfg.ssid, nw_cfg.psw);
    return true;
}

bool write_wifi(const NETWORK_CFG& nw_cfg)
{
    Preferences prefs;
    bool result = false;
    if (!prefs.begin(NVS_WIFI, false)) {
        log_d("wifi prefs begin error");
        prefs.end();
        return result;
    }
    prefs.clear();
    result = true;
    log_d("wprefs: %s %s", nw_cfg.ssid, nw_cfg.psw);
    result = prefs.putString("ssid", nw_cfg.ssid) > 0;
    result = prefs.putString("psw", nw_cfg.psw) > 0;
    if (!result) {
        log_d("wifi prefs put error");
    }
    prefs.end();
    return result;
}

bool write_player(const MPD_PLAYER& mpd_pl)
{
    Preferences prefs;
    if (!prefs.begin(NVS_PLAYER, false)) {
        log_d("players prefs begin error");
        prefs.end();
        return false;
    }
    prefs.clear();
    string key = "player";
    String data = String(
        String(mpd_pl.player_name) + "|" +
        String(mpd_pl.player_hostname) + "|" +
        String(mpd_pl.player_ip) + "|" +
        String(std::to_string(mpd_pl.player_port).c_str()));
    if (prefs.putString(key.c_str(), data) == 0) {
        log_d("players prefs put error");
        return false;
    }
    log_d("NVS write_player: %s", data.c_str());
    prefs.end();
    return true;
}

bool read_dt(RTC_DAY& day)
{
    Preferences prefs;
    if (!prefs.begin(NVS_SNTP, true)) {
        log_d("SNTP prefs begin error");
        prefs.end();
        day.day = "0000-00-00";
        return false;
    }
    String sntp_day = prefs.getString("sntp_day");
    prefs.end();
    if (sntp_day.isEmpty()) {
        log_d("empty SNTP prefs!");
        return false;
    }
    log_d("NVS read SNTP day: %s", sntp_day.c_str());
    day.day = sntp_day;
    return true;
}


bool write_dt(RTC_DAY& day)
{
    Preferences prefs;
    if (!prefs.begin(NVS_SNTP, false)) {
        log_d("SNTP prefs begin error");
        prefs.end();
        return false;
    }
    prefs.clear();
    string key = "sntp_day";
    String data = day.day;
    if (prefs.putString(key.c_str(), data) == 0) {
        log_d("SNTP prefs put error");
        return false;
    }
    log_d("NVS write SNTP day: %s", data.c_str());
    prefs.end();
    return true;
}

bool read_player(MPD_PLAYER& mpd_pl)
{
    Preferences prefs;
    if (!prefs.begin(NVS_PLAYER, true)) {
        log_d("players prefs begin error");
        prefs.end();
        return false;
    }
    string key = "player";
    String pl = prefs.getString(key.c_str());
    if (pl.isEmpty()) {
        return false;
    }
    vector<string> parts = split(string(pl.c_str()), '|');
    if (parts.size() == 4) {
        mpd_pl.player_name = strdup(parts[0].c_str());
        mpd_pl.player_hostname = strdup(parts[1].c_str());
        mpd_pl.player_ip = strdup(parts[2].c_str());
        mpd_pl.player_port = stoi(parts[3]);
    }
    else {
        log_d("invalid NVS player format: %s", pl);
    }
    log_d("NVS read_player: %s", pl.c_str());
    prefs.end();
    return true;
}
