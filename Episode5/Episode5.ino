/*************************************************************
  ESP32 MATTER SERIES – EPISODE 05
  Matter RGB Light using NeoPixel (WS2812)

  Board: XIAO ESP32 C6
  IDE  : Arduino IDE 2.3.7
  ESP32 Boards Package: v3.3.6
  Adafruit_NeoPixel verison: 1.15.4

  What this episode demonstrates:
  - Matter Color Light endpoint
  - RGB control using NeoPixel LEDs
  - HSV to RGB conversion
  - Saving last state using Preferences (NVS)
  - Button control (Toggle + Decommission)

  Building and Flashing
  - Select **"Huge APP (3MB No OTA/1MB SPIFFS)"** from **Tools > Partition Scheme** menu.
  - Enable **"Erase All Flash Before Sketch Upload"** option from **Tools** menu.

  Purchase Link (XIAO ESP32 C6)[Consider purchasing from techiesms stores to support our free educationla Videos and Codes]:
  https://techiesms.com/product/seeed-studio-xiao-esp32c6/

  YouTube Playlist (ESP32 Matter Series):
  https://www.youtube.com/techiesms

  Author: Sachin Soni

*************************************************************/

// ------------------------------------------------------------
// Libraries
// ------------------------------------------------------------

// Matter protocol library
#include <Matter.h>

// NeoPixel LED control library
#include <Adafruit_NeoPixel.h>

// Preferences library for flash storage (NVS)
#include <Preferences.h>


// ------------------------------------------------------------
// NeoPixel Configuration
// ------------------------------------------------------------

// Data pin connected to NeoPixel
#define PIXEL_PIN D2

// Number of LEDs in strip/ring
#define NUMPIXELS 2

// Store last LED state to avoid unnecessary Serial prints
uint8_t lastR = 255, lastG = 255, lastB = 255;
bool lastState = false;

// Create NeoPixel object
Adafruit_NeoPixel pixels(NUMPIXELS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);


// ------------------------------------------------------------
// Matter Endpoint
// ------------------------------------------------------------

// Create Matter Color Light endpoint
MatterColorLight ColorLight;


// ------------------------------------------------------------
// Preferences (NVS Storage)
// ------------------------------------------------------------

// Preferences object
Preferences matterPref;

// Keys used to store data
const char *onOffPrefKey = "OnOff";
const char *hsvColorPrefKey = "HSV";


// ------------------------------------------------------------
// Button Configuration
// ------------------------------------------------------------

// BOOT button used for toggle & reset
const uint8_t buttonPin = BOOT_PIN;

// Button handling variables
uint32_t button_time_stamp = 0;
bool button_state = false;
const uint32_t debouceTime = 250;              // Short press debounce time
const uint32_t decommissioningTimeout = 5000;  // Long press duration for decommission


// ------------------------------------------------------------
// Matter Callback Function
// Called whenever:
// - Light is turned ON/OFF
// - Color is changed from Matter controller
// ------------------------------------------------------------
bool setLightState(bool state, espHsvColor_t colorHSV) {

  // Convert HSV color (Matter format) to RGB
  espRgbColor_t rgb = espHsvColorToRgbColor(colorHSV);

  // Print only if state or color has changed
  if (state != lastState || rgb.r != lastR || rgb.g != lastG || rgb.b != lastB) {

    if (state) {
      Serial.printf("RGB: %d %d %d\n", rgb.r, rgb.g, rgb.b);
    } else {
      Serial.println("Light OFF");
    }

    // Save current state for next comparison
    lastR = rgb.r;
    lastG = rgb.g;
    lastB = rgb.b;
    lastState = state;
  }

  // ----------------------------------------------------------
  // Apply color to NeoPixels
  // ----------------------------------------------------------

  if (state) {
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(rgb.r, rgb.g, rgb.b));
    }
    pixels.show();
  } else {
    pixels.clear();
    pixels.show();
  }

  // ----------------------------------------------------------
  // Save state in flash memory (NVS)
  // ----------------------------------------------------------

  // Save ON/OFF state
  matterPref.putBool(onOffPrefKey, state);

  // Save HSV color as single 32-bit value
  matterPref.putUInt(hsvColorPrefKey,
                     colorHSV.h << 16 | colorHSV.s << 8 | colorHSV.v);

  return true;  // Return success to Matter core
}


// ------------------------------------------------------------
// Setup Function (Runs once at boot)
// ------------------------------------------------------------
void setup() {

  // Start Serial communication
  Serial.begin(115200);

  // Configure BOOT button
  pinMode(buttonPin, INPUT_PULLUP);

  // Initialize NeoPixel LEDs
  pixels.begin();
  pixels.clear();
  pixels.show();

  // Initialize Preferences storage
  matterPref.begin("MatterPrefs", false);

  // ----------------------------------------------------------
  // Restore last saved state from NVS
  // ----------------------------------------------------------

  bool lastOnOffState = matterPref.getBool(onOffPrefKey, true);

  uint32_t prefHsvColor =
    matterPref.getUInt(hsvColorPrefKey, 169 << 16 | 254 << 8 | 254);

  // Convert stored integer back to HSV structure
  espHsvColor_t lastHsvColor = {
    uint8_t(prefHsvColor >> 16),
    uint8_t(prefHsvColor >> 8),
    uint8_t(prefHsvColor)
  };

  // ----------------------------------------------------------
  // Initialize Matter Color Light endpoint
  // ----------------------------------------------------------

  ColorLight.begin(lastOnOffState, lastHsvColor);

  // Register callback function
  ColorLight.onChange(setLightState);

  // Start Matter stack
  Matter.begin();

  // If already commissioned, sync LED state
  if (Matter.isDeviceCommissioned()) {
    Serial.println("Matter ready");
    ColorLight.updateAccessory();
  }
}


// ------------------------------------------------------------
// Loop Function (Runs continuously)
// ------------------------------------------------------------
void loop() {

  // ----------------------------------------------------------
  // Wait until device is commissioned
  // ----------------------------------------------------------
  if (!Matter.isDeviceCommissioned()) {

    Serial.println("Waiting for commissioning...");
    Serial.printf("Manual pairing code: %s\r\n", Matter.getManualPairingCode().c_str());
    Serial.printf("QR code URL: %s\r\n", Matter.getOnboardingQRCodeUrl().c_str());

    // Block until device is paired
    while (!Matter.isDeviceCommissioned()) {
      delay(500);
    }

    // Sync LED state after commissioning
    ColorLight.updateAccessory();
  }

  // ----------------------------------------------------------
  // Button Handling
  // ----------------------------------------------------------

  // Detect button press
  if (digitalRead(buttonPin) == LOW && !button_state) {
    button_time_stamp = millis();
    button_state = true;
  }

  uint32_t time_diff = millis() - button_time_stamp;

  // Short press → Toggle light
  if (digitalRead(buttonPin) == HIGH && button_state && time_diff > debouceTime) {

    ColorLight.toggle();
    button_state = false;
  }

  // Long press → Decommission device
  if (button_state && time_diff > decommissioningTimeout) {

    Serial.println("Decommissioning...");

    // Turn OFF light before reset
    ColorLight = false;

    // Remove device from Matter network
    Matter.decommission();

    button_time_stamp = millis();
  }
}