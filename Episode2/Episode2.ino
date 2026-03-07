/*************************************************************
  ESP32 MATTER SERIES – EPISODE 02
  Matter On/Off Light with State Persistence & Button Control

  Board: XIAO ESP32 C6
  IDE  : Arduino IDE 2.3.7
  ESP32 Boards Package: v3.3.6

  What this code does:
  - Creates a Matter On/Off Light endpoint
  - Controls the onboard LED via Matter controller apps
  - Single Press Boot Button to toggle the Light & Send Realtime data
  - Supports commissioning & decommissioning
  - Long Press BOOT button to factory reset (decommission)
  - Stores the last state in NVS 

  Building and Flashing
  - Select **"Huge APP (3MB No OTA/1MB SPIFFS)"** from **Tools > Partition Scheme** menu.
  - Enable **"Erase All Flash Before Sketch Upload"** option from **Tools** menu.

  Purchase Link[Consider purchasing from techiesms stores to support our free educationla Videos and Codes]:
  XIAO ESP32 C6 - https://techiesms.com/product/seeed-studio-xiao-esp32c6/
  5V 1Channel Relay Module - https://techiesms.com/product/1-channel-5v-relay-module-with-optocoupler/
  Pushbuttons - https://techiesms.com/product/6-x-6-x-8mm-tactile-pushbutton-blue/
  840pts Breadboard - https://techiesms.com/product/gl-12-840-points-solderless-breadboard/

  YouTube Playlist (ESP32 Matter Series):
  https://www.youtube.com/techiesms

  Author: Sachin Soni
*************************************************************/

// Matter core library (handles Matter protocol)
#include <Matter.h>

// Preferences library for storing data in flash (NVS)
#include <Preferences.h>

// ------------------------------------------------------------
// Matter Endpoint Declaration
// ------------------------------------------------------------

// Create a Matter On/Off Light endpoint
MatterOnOffLight OnOffLight;

// ------------------------------------------------------------
// Preferences (Non-Volatile Storage)
// ------------------------------------------------------------

// Preferences object to store Matter related values
Preferences matterPref;
const char *onOffPrefKey = "OnOff"; // Key used to store ON/OFF state in flash memory

// ------------------------------------------------------------
// GPIO Pin Definitions
// ------------------------------------------------------------

// Onboard LED pin (acts as our light)
const uint8_t ledPin = LED_BUILTIN;

// Onboard BOOT button pin (used as toggle & reset button)
const uint8_t buttonPin = BOOT_PIN;

// ------------------------------------------------------------
// Button Handling Variables
// ------------------------------------------------------------


uint32_t button_time_stamp = 0; // Timestamp when button is pressed (used for debounce & long press)
bool button_state = false; // false = released, true = pressed
const uint32_t debouceTime = 250; // Minimum press time to consider a valid button press (ms)
const uint32_t decommissioningTimeout = 5000; // Time required to keep button pressed to decommission device (ms)

// ------------------------------------------------------------
// Matter On/Off Callback Function
// ------------------------------------------------------------

// This function is called whenever the light state changes
// from a Matter controller (Alexa, Google Home, Apple Home, etc.)
bool setLightOnOff(bool state) {

  // Print new state to Serial Monitor
  Serial.printf("User Callback :: New Light State = %s\r\n",
                state ? "ON" : "OFF");

  // Control the physical LED
  if (state) {
    digitalWrite(ledPin, LOW);
  } else {
    digitalWrite(ledPin, HIGH);
  }

  // Store the current ON/OFF state in flash memory
  // This allows state recovery after power loss or reboot
  matterPref.putBool(onOffPrefKey, state);

  // Return true to indicate successful execution
  return true;
}

// ------------------------------------------------------------
// Setup Function (Runs once at boot)
// ------------------------------------------------------------

void setup() {

  // Configure BOOT button as input with internal pull-up
  pinMode(buttonPin, INPUT_PULLUP);

  // Configure LED pin as output
  pinMode(ledPin, OUTPUT);

  // Start Serial communication for debugging
  Serial.begin(115200);

  // Initialize Preferences storage namespace
  matterPref.begin("MatterPrefs", false);

  // Read last stored ON/OFF state (default = ON)
  bool lastOnOffState = matterPref.getBool(onOffPrefKey, true);

  // Initialize Matter Light endpoint with saved state
  OnOffLight.begin(lastOnOffState);

  // Register callback for Matter ON/OFF changes
  OnOffLight.onChange(setLightOnOff);

  // Start Matter stack (must be last step)
  Matter.begin();

  // This may be a restart of a already commissioned Matter accessory
  if (Matter.isDeviceCommissioned()) {
    Serial.println("Matter Node is commissioned and connected to the network. Ready for use.");
    Serial.printf("Initial state: %s\r\n", OnOffLight.getOnOff() ? "ON" : "OFF");
    OnOffLight.updateAccessory();  // configure the Light based on initial state
  }
}

void loop() {

  // ----------------------------------------------------------
  // Check commissioning status
  // ----------------------------------------------------------

  // Check Matter Light Commissioning state, which may change during execution of loop()
  if (!Matter.isDeviceCommissioned()) {
    Serial.println("");
    Serial.println("Matter Node is not commissioned yet.");
    Serial.println("Initiate the device discovery in your Matter environment.");
    Serial.println("Commission it to your Matter hub with the manual pairing code or QR code");
    Serial.printf("Manual pairing code: %s\r\n", Matter.getManualPairingCode().c_str());
    Serial.printf("QR code URL: %s\r\n", Matter.getOnboardingQRCodeUrl().c_str());
    // waits for Matter Light Commissioning.
    uint32_t timeCount = 0;
    while (!Matter.isDeviceCommissioned()) {
      delay(100);
      if ((timeCount++ % 50) == 0) {  // 50*100ms = 5 sec
        Serial.println("Matter Node not commissioned yet. Waiting for commissioning.");
      }
    }
    Serial.printf("Initial state: %s\r\n", OnOffLight.getOnOff() ? "ON" : "OFF");
    OnOffLight.updateAccessory();  // configure the Light based on initial state
    Serial.println("Matter Node is commissioned and connected to the network. Ready for use.");
  }

  // ----------------------------------------------------------
  // Button Press Detection
  // ----------------------------------------------------------

  // Detect initial button press
  if (digitalRead(buttonPin) == LOW && !button_state) {
    button_time_stamp = millis();  // Save press time
    button_state = true;           // Mark as pressed
  }

  // Calculate how long the button has been pressed
  uint32_t time_diff = millis() - button_time_stamp;

  // ----------------------------------------------------------
  // Short Press → Toggle Light
  // ----------------------------------------------------------

  // Button released after debounce time
  if (button_state && time_diff > debouceTime && digitalRead(buttonPin) == HIGH) {

    button_state = false;  // Mark as released

    Serial.println("User button released. Toggling Light!");

    // Toggle light state (visible to Matter controller)
    OnOffLight.toggle();
  }

  // ----------------------------------------------------------
  // Long Press → Decommission Device
  // ----------------------------------------------------------

  // If button is held for more than 5 seconds
  if (button_state && time_diff > decommissioningTimeout) {

    Serial.println("Decommissioning Matter Light Accessory.");

    // Turn OFF the light before reset
    OnOffLight.setOnOff(false);

    // Factory reset / remove from Matter network
    Matter.decommission();

    // Reset timestamp to avoid repeated calls
    button_time_stamp = millis();
  }
}
