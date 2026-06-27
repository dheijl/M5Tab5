/*

M5 AI support provided "workaround" for unimplemented LightSleep in Arduino M5Unified Power.

Doesn't work either, probably because the pin is not wired in hardware...

Anyway, here it comes:
*/

/* M5Support (AI) providede code:

Important caveats
- Required versions: Use ESP32 Arduino board package ≥ 3.0.6 (earlier 3.0.x releases had broken ESP32-P4 light sleep),
M5Unified ≥ 0.2.22, and M5GFX ≥ 0.2.26.
- Power draw: Current ESP32-P4 Arduino light sleep draws more current than the ~0.5–3mA typical on ESP32-S3,
as peripheral auto-gating is still being optimized. Expect lower draw if you disable WiFi/BLE before sleep.
- Deep sleep alternative: For ~10–150µA low power (with full chip reset on wake),
M5Tab5 currently only officially supports timer wakeup (internal RTC or onboard RX8130 external RTC via M5.Power.timerSleep()).
Deep sleep touch wake requires an LP-domain RTC GPIO, and GPIO23 is not confirmed to be LP-capable on ESP32-P4.
- The touch controller and I2C bus remain powered during light sleep, so you do not need to re-initialize M5.Touch after wakeup.

*/


#include <M5Unified.h>
// Only add these if you get compile errors:
// #include "esp_sleep.h"
// #include "driver/gpio.h"

void enterLightSleepTouchWake(uint64_t timer_wake_us = 0) {
    // Optional: Disable wireless peripherals for much lower power draw
    // WiFi.end();
    // BLE.end(); // uncomment if using BLE

    // Power off display (matches M5Unified internal sleep behavior)
    M5.Display.sleep();
    M5.Display.waitDisplay();

    // Configure TP_INT (GPIO23, active low) as light sleep wake source
    pinMode(23, INPUT_PULLUP); // Tab5 has external pull-up on TP_INT
    gpio_wakeup_enable(GPIO_NUM_23, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();

    // Optional: Add timed auto-wake in addition to touch
    if (timer_wake_us > 0) {
        esp_sleep_enable_timer_wakeup(timer_wake_us);
    }

    // Wait for TP_INT to be released to avoid immediate wakeup
    while (digitalRead(23) == LOW) { delay(1); }

    // Enter light sleep (execution pauses here until wake)
    esp_light_sleep_start();

    // --- Runs immediately after wake ---
    // Clean up wakeup source
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO);
    if (timer_wake_us > 0) {
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
    }

    // Restore display
    M5.Display.wakeup();
    delay(100); // wait for display PLL to stabilize
}
/*
void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setFont(&fonts::FreeMonoBold18pt7b);
    M5.Display.setRotation(1);
    M5.Display.clear();
    M5.Display.drawString("Touch to enter sleep", M5.Display.width() / 2, M5.Display.height() / 2);
}

void loop() {
    M5.update();
    if (M5.Touch.getDetail(0).wasClicked()) {
        enterLightSleepTouchWake(); // wait indefinitely for touch wake
        // For 30s timeout + touch wake: enterLightSleepTouchWake(30000000ULL);

        // Code here runs immediately after wake
        M5.Display.clear();
        M5.Display.drawString("Woken by touch!", M5.Display.width() / 2, M5.Display.height() / 2);
        delay(1000);
        M5.Display.clear();
        M5.Display.drawString("Touch to re-enter sleep", M5.Display.width() / 2, M5.Display.height() / 2);
    }
}
*/