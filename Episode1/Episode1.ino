/*
  ============================================================
  ESP32 MATTER SERIES – EPISODE 01
  Single On/Off Light using Matter Protocol
  ============================================================

  This code is part of the FIRST EPISODE of our ESP32 Matter
  video series, where we build a Matter-enabled IoT device
  from scratch.

  Hardware Used:
  - Seeed Studio XIAO ESP32 C6

  Software Environment:
  - Arduino IDE version: 2.3.7
  - ESP32 Boards Package version: 3.3.6

  What this code does:
  - Creates a Matter On/Off Light endpoint
  - Controls the onboard LED via Matter controller apps
  - Supports commissioning & decommissioning
  - Uses BOOT button to factory reset (decommission)

  Building and Flashing
  - Select **"Huge APP (3MB No OTA/1MB SPIFFS)"** from **Tools > Partition Scheme** menu.
  - Enable **"Erase All Flash Before Sketch Upload"** option from **Tools** menu.

  Purchase Link (XIAO ESP32 C6)[Consider purchasing from techiesms stores to support our free educationla Videos and Codes]:
  https://techiesms.com/product/seeed-studio-xiao-esp32c6/

  YouTube Playlist (ESP32 Matter Series):
  https://www.youtube.com/techiesms

  Author: Sachin Soni
  ============================================================
*/

// Matter Manager Library
#include <Matter.h>

// List of Matter Endpoints for this Node

// Single On/Off Light Endpoint - at least one per node
MatterOnOffLight OnOffLight;

// Light GPIO that can be controlled by Matter APP
const uint8_t ledPin = LED_BUILTIN;

// set your board USER BUTTON pin here - decommissioning button
const uint8_t buttonPin = BOOT_PIN;  // Set your pin here. Using BOOT Button.

// Button control - decommision the Matter Node
uint32_t button_time_stamp = 0;                // debouncing control
bool button_state = false;                     // false = released | true = pressed
const uint32_t decommissioningTimeout = 5000;  // keep the button pressed for 5s, or longer, to decommission

// Matter Protocol Endpoint (On/OFF Light) Callback
bool onOffLightCallback(bool state) {
  digitalWrite(ledPin, state ? LOW : HIGH);
  Serial.println("LED State - " + (String) state);
  // This callback must return the success state to Matter core
  return true;
}

void setup() 
{
  Serial.begin(115200);

  // Initialize the USER BUTTON (Boot button) that will be used to decommission the Matter Node
  pinMode(buttonPin, INPUT_PULLUP);
  // Initialize the LED GPIO
  pinMode(ledPin, OUTPUT);

  // Initialize at least one Matter EndPoint
  OnOffLight.begin();

  // Associate a callback to the Matter Controller
  OnOffLight.onChange(onOffLightCallback);

  // Matter beginning - Last step, after all EndPoints are initialized
  Matter.begin();

  if (!Matter.isDeviceCommissioned()) {
    Serial.println("Matter Node is not commissioned yet.");
    Serial.println("Initiate the device discovery in your Matter environment.");
    Serial.println("Commission it to your Matter hub with the manual pairing code or QR code");
    Serial.printf("Manual pairing code: %s\r\n", Matter.getManualPairingCode().c_str());
    Serial.printf("QR code URL: %s\r\n", Matter.getOnboardingQRCodeUrl().c_str());
  }
}

void loop() {
  // Check if the button has been pressed
  if (digitalRead(buttonPin) == LOW && !button_state) {
    // deals with button debouncing
    button_time_stamp = millis();  // record the time while the button is pressed.
    button_state = true;           // pressed.
  }

  if (digitalRead(buttonPin) == HIGH && button_state) {
    button_state = false;  // released
  }

  // Onboard User Button is kept pressed for longer than 5 seconds in order to decommission matter node
  uint32_t time_diff = millis() - button_time_stamp;
  if (button_state && time_diff > decommissioningTimeout) {
    Serial.println("Decommissioning the Light Matter Accessory. It shall be commissioned again.");
    Matter.decommission();
    button_time_stamp = millis();  // avoid running decommissining again, reboot takes a second or so
  }

  delay(500);
}
