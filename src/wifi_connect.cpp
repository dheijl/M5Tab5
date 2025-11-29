#include "wifi_connect.h"

static NETWORK_CFG cfg;

void connect_wifi()
{
    M5Canvas wifi_canvas(&M5.Display);
    wifi_canvas.setPsram(true);
    wifi_canvas.createSprite(360, 32);
    wifi_canvas.setFont(&fonts::FreeMonoBold12pt7b);
    wifi_canvas.setTextDatum(top_left);
    wifi_canvas.setTextColor(WHITE);


    if (!read_wifi(cfg)) {
        wifi_canvas.setTextColor(RED);
        wifi_canvas.drawString("no wifi config", 0, 4);
        wifi_canvas.pushSprite(360, 0);
        wifi_canvas.deleteSprite();
        vTaskDelay(10000);
        return;
    }
    else {
        log_d("NVS wifi config: %s %s", cfg.ssid, cfg.psw);
    }
    // STA MODE
    WiFi.setSleep(false);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.begin(cfg.ssid, cfg.psw);
    // Wait for connection
    uint32_t i = 0;
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(250);
        if (i % 4 == 0) {
            wifi_canvas.setTextColor(WHITE);
        }
        else {
            wifi_canvas.setTextColor(RED);
        }
        wifi_canvas.drawString(cfg.ssid, 0, 4);
        wifi_canvas.pushSprite(360, 0);
        if (++i > 100) {
            break;
        }
    }
    if (WiFi.status() == WL_CONNECTED) {
        wifi_canvas.setTextColor(GREEN);
        wifi_canvas.drawString(cfg.ssid, 0, 4);
        wifi_canvas.setTextColor(WHITE);
        wifi_canvas.drawString(WiFi.localIP().toString(), 160, 4);
    }
    else {
        wifi_canvas.setTextColor(RED);
        wifi_canvas.drawString(cfg.ssid, 0, 4);
    }
    wifi_canvas.pushSprite(360, 0);
    wifi_canvas.deleteSprite();

}

void wifiscan() {
    M5.Display.print("Wifi scan ..");
    int n = WiFi.scanNetworks();
    M5.Display.println("Scan done");
    if (n == 0) {
        M5.Display.println("no networks found");
    }
    else {
        M5.Display.print(n);
        M5.Display.println(" networks found");
        M5.Display.println("Nr | SSID                             | RSSI | CH | Encryption");
        for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI for each network found
            M5.Display.printf("%2d", i + 1);
            M5.Display.print(" | ");
            M5.Display.printf("%-32.32s", WiFi.SSID(i).c_str());
            M5.Display.print(" | ");
            M5.Display.printf("%4ld", WiFi.RSSI(i));
            M5.Display.print(" | ");
            M5.Display.printf("%2ld", WiFi.channel(i));
            M5.Display.print(" | ");
            switch (WiFi.encryptionType(i)) {
            case WIFI_AUTH_OPEN:
                M5.Display.print("open");
                break;
            case WIFI_AUTH_WEP:
                M5.Display.print("WEP");
                break;
            case WIFI_AUTH_WPA_PSK:
                M5.Display.print("WPA");
                break;
            case WIFI_AUTH_WPA2_PSK:
                M5.Display.print("WPA2");
                break;
            case WIFI_AUTH_WPA_WPA2_PSK:
                M5.Display.print("WPA+WPA2");
                break;
            case WIFI_AUTH_WPA2_ENTERPRISE:
                M5.Display.print("WPA2-EAP");
                break;
            case WIFI_AUTH_WPA3_PSK:
                M5.Display.print("WPA3");
                break;
            case WIFI_AUTH_WPA2_WPA3_PSK:
                M5.Display.print("WPA2+WPA3");
                break;
            case WIFI_AUTH_WAPI_PSK:
                M5.Display.print("WAPI");
                break;
            default:
                M5.Display.print("unknown");
            }
            M5.Display.println();
            delay(10);
        }
    }
}
