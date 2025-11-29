#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <StreamUtils.h>

#include "p1dongle.h"
#include "utils.h"

bool p1_request(P1_DATA& dongle_data) {
    WiFiClient transport;
    HTTPClient dongle;
    // Ask HTTPClient to collect the Transfer-Encoding response header
    // (by default, it discards all headers)
    const char* keys[] = { "Transfer-Encoding" };
    dongle.collectHeaders(keys, 1);

    // Send request
    dongle.begin(transport, "http://nrg-dongle-pro.local/api/v2/sm/actual");
    dongle.GET();

    // Get the raw and the decoded stream
    Stream& rawStream = dongle.getStream();
    ChunkDecodingStream decodedStream(dongle.getStream());

    // Choose the right stream depending on the Transfer-Encoding header
    Stream& response =
        dongle.header("Transfer-Encoding") == "chunked" ? decodedStream : rawStream;

    // Parse response
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        log_d("Json deserialization error %s", error.f_str());
        return false;
    }
    /*for (JsonPair kv : doc.as<JsonObject>()) {
        log_d("%s %s", kv.key().c_str(), kv.value().as<std::string>().c_str());
    }*/
    dongle_data.highest_peak_pwr = get_value(doc, "highest_peak_pwr");
    dongle_data.energy_delivered_tariff1 = get_value(doc, "energy_delivered_tariff1");
    dongle_data.energy_delivered_tariff2 = get_value(doc, "energy_delivered_tariff2");
    dongle_data.energy_returned_tariff1 = get_value(doc, "energy_returned_tariff1");
    dongle_data.energy_returned_tariff2 = get_value(doc, "energy_returned_tariff2");
    dongle_data.power_delivered = get_value(doc, "power_delivered");
    dongle_data.power_returned = get_value(doc, "power_returned");
    dongle_data.current_l1 = get_value(doc, "current_l1");
    dongle_data.current_l3 = get_value(doc, "current_l3");
    dongle_data.gas_delivered = get_value(doc, "gas_delivered");
    return true;
}

static float get_value(JsonDocument& doc, const char* key) {
    float result = 0.0;
    auto jo = doc[key].as<JsonObject>();
    result = jo["value"].as<float>();
    return result;
}