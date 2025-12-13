#ifndef __UI_H__
#define __UI_H__

#include <Arduino.h>
#include <M5Unified.h>
#include <M5GFX.h>

#include "power.h"
#include "synctime.h"
#include "mpdcli.h"
#include "p1dongle.h"

constexpr static uint32_t BG_COLOR = lgfx::color888(0, 51, 102);
constexpr static uint32_t P1_BG_COLOR = lgfx::color888(92, 83, 122);
constexpr static uint32_t CH_BG_COLOR = lgfx::color888(240, 182, 127);
constexpr static uint32_t LX_COLOR = lgfx::color888(201, 112, 38);
constexpr static uint32_t KWH_COLOR = lgfx::color888(95, 88, 199);
constexpr static uint32_t SUN_COLOR = lgfx::color888(138, 222, 173);

void display_power_ui();
void display_date_time_ui();
void display_mpd_ui(MPD_Client* mpd_cli);
void display_dongle_ui(const P1_DATA& dongle_data);

#endif