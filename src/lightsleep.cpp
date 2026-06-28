/*
If the runtime interrupt works, this sketch works around all known Tab5 issues:
it bypasses any missing _wakeupPin board configuration,
holds the touch controller out of reset during sleep,
places the touch controller in low-power wake-on-touch mode (instead of powering it off),
and explicitly configures GPIO wakeup for the non-RTC GPIO23 pin:
*/

/*Troubleshooting if it still fails:
Check the serial wake cause: if you never see ESP_SLEEP_WAKEUP_GPIO,
the touch controller is not asserting INT (you may need to adjust the low-power register value if you have the ST7123 panel variant).
If you see immediate repeated wake, the INT line is stuck LOW: add a 50-100ms debounce before entering sleep and extend the pre-sleep INT release wait loop.
If the IO expander writes fail, confirm you are using the latest M5Unified where M5.In_I2C is correctly mapped for your Tab5 revision.
As an officially supported fallback if touch wake continues to be problematic, you can use:

M5.Power.timerSleep(wakeTime) with the on-board RX8130CE external RTC for ultra-low power timed wake
BMI270 IMU motion interrupt wakeup (officially supported for Tab5 light/deep sleep)
*/


#include <M5Unified.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
#include <Wire.h>

// M5TAB5 hardware constants
static constexpr gpio_num_t TP_INT_PIN = GPIO_NUM_23;
static constexpr uint8_t IO_EXP_ADDR = 0x43;     // PI4IOE5V6408 internal IO expander
static constexpr uint8_t TP_RST_EXP_PIN = 5;     // Expander pin E1.P5 = touch reset (active LOW)
static constexpr uint8_t GT911_ADDR = 0x14;      // GT911 touch I2C address
static constexpr uint8_t ST7123_ADDR = 0x55;     // ST7123/ST7121 touch I2C address

// Configure PI4IOE5V6408 expander to keep touch controller out of reset during sleep
void keepTouchOutOfReset() {
    TwoWire& exp_i2c = M5.In_I2C; // Use M5Unified's pre-configured internal I2C bus
    // Set TP_RST pin as output
    exp_i2c.beginTransmission(IO_EXP_ADDR);
    exp_i2c.write(0x03); // Direction register
    exp_i2c.write(~(1 << TP_RST_EXP_PIN) & 0xFF); // Pin 5 = output, all others input
    exp_i2c.endTransmission();
    // Set TP_RST HIGH (release reset)
    exp_i2c.beginTransmission(IO_EXP_ADDR);
    exp_i2c.write(0x05); // Output state register
    exp_i2c.write(1 << TP_RST_EXP_PIN);
    exp_i2c.endTransmission();
    // Disable pull resistors on TP_RST
    exp_i2c.beginTransmission(IO_EXP_ADDR);
    exp_i2c.write(0x0B); // Pull disable register
    exp_i2c.write(1 << TP_RST_EXP_PIN);
    exp_i2c.endTransmission();
}

// Put touch controller into low-power mode that asserts INT on touch
void enableTouchWakeOnInt() {
    Wire.begin(31, 32); // Touch I2C bus (per official Tab5 pinout)
    // Configure GT911 if present
    Wire.beginTransmission(GT911_ADDR);
    if (Wire.endTransmission() == 0) {
        Wire.beginTransmission(GT911_ADDR);
        Wire.write(0x80); Wire.write(0x40); // Command register
        Wire.write(0x02); // Enter Green Mode (~1mA, INT pulls low on touch)
        Wire.endTransmission();
        return;
    }
    // Configure ST7123/ST7121 if present
    Wire.beginTransmission(ST7123_ADDR);
    if (Wire.endTransmission() == 0) {
        Wire.beginTransmission(ST7123_ADDR);
        Wire.write(0x10); // Power control register
        Wire.write(0x01); // Enter standby mode with touch INT wake enabled
        Wire.endTransmission();
    }
}

void lightSleepTouchWake(uint64_t sleep_us = 0) {
    keepTouchOutOfReset();
    M5.Display.setBrightness(0);
    M5.Display.sleep();
    M5.Display.waitDisplay();
    enableTouchWakeOnInt(); // Re-configure touch after display sleep (avoids power-off)

    // Wait for touch INT to return HIGH to prevent immediate wake
    pinMode(TP_INT_PIN, INPUT);
    while (digitalRead(TP_INT_PIN) == LOW) { delay(5); }

    // Explicit GPIO wakeup config (bypasses M5Unified board definition bugs)
    gpio_wakeup_enable(TP_INT_PIN, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();
    if (sleep_us > 0) {
        esp_sleep_enable_timer_wakeup(sleep_us); // Optional safety timer
    }

    esp_light_sleep_start(); // Enter sleep

    // Post-wake cleanup
    gpio_wakeup_disable(TP_INT_PIN);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);

    M5.Display.wakeup();
    M5.Touch.begin(); // Re-initialize touch after low-power mode
    M5.Display.setBrightness(200); // Restore your default brightness
}

/*
void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    Serial.begin(115200);
    M5.Display.setRotation(1);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setFont(&fonts::FreeMonoBold18pt7b);
    M5.Display.clear();
    M5.Display.drawString("Touch to enter light sleep", M5.Display.width() / 2, M5.Display.height() / 2);
}

void loop() {
    M5.update();
    auto touch = M5.Touch.getDetail(0);
    if (touch.wasClicked()) {
        Serial.println("Entering light sleep...");
        delay(100); // Flush serial
        lightSleepTouchWake(); // Indefinite wake-on-touch; add ms value for timer fallback

        // Print wake cause for debugging
        auto cause = esp_sleep_get_wakeup_cause();
        if (cause == ESP_SLEEP_WAKEUP_GPIO) {
            Serial.println("✅ Woke from touch (GPIO interrupt)");
        }
        else if (cause == ESP_SLEEP_WAKEUP_TIMER) {
            Serial.println("Woke from timer fallback");
        }
        else {
            Serial.printf("Woke from unexpected cause: %d\n", cause);
        }

        M5.Display.clear();
        M5.Display.drawString("Woke up! Touch to sleep again", M5.Display.width() / 2, M5.Display.height() / 2);
    }
}
*/