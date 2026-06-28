/*

M5 AI support provided "workaround" for unimplemented LightSleep in Arduino M5Unified Power.

The root cause is almost certainly that M5Unified's default lightSleep(0, true) path tries to use
EXT0/RTC wakeup on _wakeupPin, but GPIO23 (TP_INT) on ESP32-P4 is not an RTC-capable pin,
so the interrupt cannot fire in light sleep the same way it does on boards with an RTC touch-int pin.
his is the same class of issue documented for M5PaperS3 (GPIO48), and the library does not currently
ship a Tab5-specific workaround.

What we know about M5TAB5
MCU: ESP32-P4
Touch controller: GT911 (0x14) / ST7123 / ST7121 (0x55)
Touch INT pin: GPIO23 (TP_INT, active LOW)
Touch I2C: SDA = G31, SCL = G32
Touch reset is via the PI4IOE5V6408 IO expander (E1.P5 / TP_RST)
The official M5TAB5 deep-sleep example uses deepSleep(..., false) (i.e. touch wake disabled),
which is a strong hint that native touch-pin wake via EXT0 is not reliable/supported on this board.
Working workaround: use GPIO wakeup (not EXT0) in light sleep
Configure GPIO23 with gpio_wakeup_enable(..., GPIO_INTR_LOW_LEVEL) instead of esp_sleep_enable_ext0_wakeup(...).
GPIO wakeup works in light sleep for non-RTC digital pins like GPIO23.touch controller
and I2C bus remain powered during light sleep, so you do not need to re-initialize M5.Touch after wakeup.

Additional tips
Do not call gpio_pullup_en(GPIO23) unless you verify it doesn't conflict with the GT911/ST7123 INT driver;
the touch controller already drives the line.
If you see immediate re-wake, that is almost always the "wait for release" step missing — add the while
(gpio_get_level(GPIO_NUM_23) == 0) loop exactly as shown.
If you need lower power than light sleep and touch wake isn't mandatory, use RTC timer sleep (M5.Power.timerSleep(...)
via the on-board RX8130CE) which is fully supported on Tab5.
Deep sleep on Tab5 is best used with timer wakeup (M5.Power.deepSleep(us, false));
EXT0/EXT1 touch-pin wake from deep sleep is not expected to work on GPIO23.

*/


#include <M5Unified.h>
#include <esp_sleep.h>
#include <driver/gpio.h>

void lightSleepTouchWake(uint64_t sleep_us = 0) {
    // Let display settle then put it to sleep
    M5.Display.sleep();
    M5.Display.waitDisplay();

    // Configure TP_INT (GPIO23, active low) as light sleep wake source
    // pinMode(23, INPUT_PULLUP); // Tab5 has external pull-up on TP_INT

    // Critical: wait until TP_INT is released (HIGH) so we don't wake instantly
    while (gpio_get_level(GPIO_NUM_23) == 0) {
        delay(5);
    }

    // GPIO wakeup on GPIO23, active-LOW (touch INT pulls low)
    gpio_wakeup_enable(GPIO_NUM_23, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();

    if (sleep_us > 0) {
        esp_sleep_enable_timer_wakeup(sleep_us);
    }

    esp_light_sleep_start();

    // Cleanup after wakeup
    gpio_wakeup_disable(GPIO_NUM_23);
    M5.Display.wakeup();
}

/*
void loop() {
    M5.update();
    if (M5.Touch.getDetail(0).wasClicked()) {
        lightSleepTouchWake();  // indefinite, wakes on touch
        // or: lightSleepTouchWake(10ULL * 1000000);  // + 10s safety timer
    }
}
*/