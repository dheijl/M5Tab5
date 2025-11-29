#ifndef __NVS_CONFIG__H__
#define __NVS_CONFIG__H__

#include <Preferences.h>
#include <string.h>
#include <string>

#include "utils.h"

typedef struct network_cfg {
    const char* ssid;
    const char* psw;
} NETWORK_CFG;

typedef struct mpd_player {
    const char* player_name;
    const char* player_hostname;
    const char* player_ip;
    uint16_t player_port;
} MPD_PLAYER;

typedef struct rtc_day {
    String day;
} RTC_DAY;

static const constexpr char* NVS_WIFI = "wifi";
static const constexpr char* NVS_PLAYER = "player";
static const constexpr char* NVS_SNTP = "SNTP_DAY";
static const constexpr char* NVS_FAVS = "favs";

bool read_wifi(NETWORK_CFG& nw_cfg);
bool write_wifi(const NETWORK_CFG& nw_cfg);
bool read_player(MPD_PLAYER& player);
bool write_player(const MPD_PLAYER& player);
bool read_dt(RTC_DAY& dt);
bool write_dt(RTC_DAY& dt);



#endif
