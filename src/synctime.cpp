// Copyright (c) 2023 @dheijl (danny.heijl@telenet.be)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// The M5Stack Arduino TAB5 SNTP code does not work
// working esp32 SNTP code borrowed from https://www.makerguides.com/how-to-synchronize-esp32-clock-with-sntp-server/
#include <Arduino.h>
#include <M5Unified.h>
#include <utility/rtc/RTC_Base.hpp>
#include <esp_sntp.h>

#include "synctime.h"
#include "nvs_config.h"

static void notify(struct timeval* t) {
    struct tm dt;
    getLocalTime(&dt);

    m5::rtc_date_t date_struct;
    date_struct.weekDay = dt.tm_wday;
    date_struct.month = dt.tm_mon + 1;
    date_struct.date = dt.tm_mday;
    date_struct.year = dt.tm_year + 1900;
    M5.Rtc.setDate(&date_struct);
    m5::rtc_time_t time_struct;
    time_struct.hours = dt.tm_hour;
    time_struct.minutes = dt.tm_min;
    time_struct.seconds = dt.tm_sec;
    M5.Rtc.setTime(&time_struct);
    m5::rtc_date_t date;
    m5::rtc_time_t time;
    bool rc = (M5.Rtc.getDate(&date) & M5.Rtc.getTime(&time));
    if (rc) {
        log_d("Date: %04d/%02d/%02d - ", date.year, date.month, date.date);
        log_d("Time: %02d:%02d:%02d\n", time.hours, time.minutes, time.seconds);
    }
    Serial.println("Time synchronized");
}

static void setTimezone(const char* tz) {
    setenv("TZ", tz, 1);
    tzset();
}

static void initSNTP(const char* tz) {
    sntp_set_sync_interval(3 * 60 * 60 * 1000UL);  // 3 hours
    sntp_set_time_sync_notification_cb(notify);
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "0.be.pool.ntp.org");
    esp_sntp_setservername(1, "0.europe.pool.ntp.org");
    esp_sntp_init();
    setTimezone(tz);
}

static void wait4SNTP() {
    while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) {
        vTaskDelay(250);
        log_d("waiting for SNTP...");
    }
}

// SNTP sync is needed at least once every day
// the date stored in NVS prefs is compared with the RTC date 
// datetime is SNTP synced if different, and NVS updated
void check_time() {
    m5::rtc_date_t d;
    m5::rtc_time_t t;
    bool rc = (M5.Rtc.getDate(&d) & M5.Rtc.getTime(&t));
    if (!rc) {
        sync_time();
        M5.Rtc.getDate(&d);
        M5.Rtc.getTime(&t);
    }
    RTC_DAY dt;
    String rtc_day = String(d.year) + "-" + String(d.month) + "-" + String(d.date);
    if (!read_dt(dt)) {
        sync_time();
        M5.Rtc.getDate(&d);
        M5.Rtc.getTime(&t);
        rtc_day = String(d.year) + "-" + String(d.month) + "-" + String(d.date);
        dt.day = rtc_day;
        if (!write_dt(dt)) {
            log_d("error writing NVS SNTP day");
        }
    }
    else {
        if (rtc_day != dt.day) {
            sync_time();
            log_d("Time synced");
            M5.Rtc.getDate(&d);
            M5.Rtc.getTime(&t);
            rtc_day = String(d.year) + "-" + String(d.month) + "-" + String(d.date);
            dt.day = rtc_day;
            if (!write_dt(dt)) {
                log_d("error writing NVS SNTP day");
            }
        }
    }
}

// Initialize the RTC time with SNTP
// The M5 Arduino configTzTime() does not work at all
// But the esp32 code above does work
void sync_time()
{
    initSNTP("CET-1CEST,M3.5.0/02:00:00,M10.5.0/03:00:00");
    wait4SNTP();
    struct tm dt;
    getLocalTime(&dt);
    m5::rtc_date_t date_struct;
    date_struct.weekDay = dt.tm_wday;
    date_struct.month = dt.tm_mon + 1;
    date_struct.date = dt.tm_mday;
    date_struct.year = dt.tm_year + 1900;
    M5.Rtc.setDate(&date_struct);
    m5::rtc_time_t time_struct;
    time_struct.hours = dt.tm_hour;
    time_struct.minutes = dt.tm_min;
    time_struct.seconds = dt.tm_sec;
    M5.Rtc.setTime(&time_struct);
    m5::rtc_date_t d;
    m5::rtc_time_t t;
    M5.Rtc.getDate(&d);
    M5.Rtc.getTime(&t);
    log_d("SNTP: %04d/%02d/%02d - %02d:%02d:%02d", d.year, d.month, d.date, t.hours, t.minutes, t.seconds);

}
