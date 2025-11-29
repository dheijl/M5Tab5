#ifndef __P1__DONGLE__H__
#define __P1__DONGLE__H__

#include <ArduinoJson.h>


typedef struct p1_value {
    float value;
    String Unit;
} P1_VALUE;

typedef struct p1_data {
    float highest_peak_pwr;
    float energy_delivered_tariff1;
    float energy_delivered_tariff2;
    float energy_returned_tariff1;
    float energy_returned_tariff2;
    float power_delivered;
    float power_returned;
    float current_l1;
    float current_l3;
    float gas_delivered;
} P1_DATA;

static float get_value(JsonDocument& doc, const char* key);
bool p1_request(P1_DATA& dongle_data);
#endif