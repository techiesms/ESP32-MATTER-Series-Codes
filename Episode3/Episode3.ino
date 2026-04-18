/*************************************************************
  ESP32 MATTER SERIES – EPISODE 03
  Matter Smart Button

  Board: XIAO ESP32 C6
  IDE  : Arduino IDE 2.3.7
  ESP32 Boards Package: v3.3.6

  What this episode demonstrates:
  - Matter Smart Button endpoint
  - Sending a "click" event to Matter Controller
  - Using onboard BOOT button as a smart button
  - Long press to decommission Matter device

  Use case:
  - Trigger automations (lights, scenes, routines)
  - Customised Alexa Announcements 
  - Act as a wireless Matter-compatible button for any purpose


*************************************************************/

// ------------------------------------------------------------
// Matter Core Library
// ------------------------------------------------------------

// Matter Manager (handles Matter stack & communication)
#include <Matter.h>

// ------------------------------------------------------------
// Matter Endpoint Declaration
// ------------------------------------------------------------

// Generic Switch endpoint
// Works like a smart button (single-click action)
MatterGenericSwitch SmartButton;

// ------------------------------------------------------------
// GPIO Pin Definitions
// ------------------------------------------------------------

// Onboard BOOT button pin
// Used as smart button & decommission trigger
const uint8_t buttonPin = BOOT_PIN;

// ------------------------------------------------------------
// Button Handling Variables
// ------------------------------------------------------------

uint32_t button_time_stamp = 0;                // Timestamp when button press is detected
bool button_state = false;                     // false = released, true = pressed
const uint32_t debouceTime = 250;              // Minimum press duration to filter button bounce (ms)
const uint32_t decommissioningTimeout = 5000;  // Time required to hold button for decommissioning (ms)

// ------------------------------------------------------------
// Setup Function (Runs once on boot)
// ------------------------------------------------------------

void setup() {

  Serial.begin(115200);
  // Configure BOOT button as input with internal pull-up resistor
  pinMode(buttonPin, INPUT_PULLUP);

  // Initialize the Matter Generic Switch endpoint
  SmartButton.begin();

  // Start the Matter stack
  // IMPORTANT: Must be called after all endpoints are initialized
  Matter.begin();

  // Check if device was already commissioned earlier
  if (Matter.isDeviceCommissioned()) {
    Serial.println("Matter Node is commissioned and connected to the network.");
    Serial.println("Generic Switch is ready to send events.");
  }
}

// ------------------------------------------------------------
// Loop Function (Runs continuously)
// ------------------------------------------------------------

void loop() {

  // ----------------------------------------------------------
  // Matter Commissioning Check
  // ----------------------------------------------------------

  // If the device is not commissioned yet
  if (!Matter.isDeviceCommissioned()) {

    Serial.println("");
    Serial.println("Matter Node is not commissioned yet.");
    Serial.println("Start commissioning from your Matter controller.");
    Serial.printf("Manual pairing code: %s\r\n",
                  Matter.getManualPairingCode().c_str());
    Serial.printf("QR code URL: %s\r\n",
                  Matter.getOnboardingQRCodeUrl().c_str());

    // Wait here until commissioning is completed
    uint32_t timeCount = 0;
    while (!Matter.isDeviceCommissioned()) {
      delay(100);
      if ((timeCount++ % 50) == 0) {
        Serial.println("Waiting for Matter commissioning...");
      }
    }

    Serial.println("Matter Node is commissioned and connected.");
    Serial.println("Generic Switch is ready for use.");
  }

  // ----------------------------------------------------------
  // Button Press Detection
  // ----------------------------------------------------------

  // Detect initial button press (LOW = pressed)
  if (digitalRead(buttonPin) == LOW && !button_state) {

    // Store the time when button is pressed
    button_time_stamp = millis();

    // Mark button as pressed
    button_state = true;
  }

  // Calculate how long the button has been pressed
  uint32_t time_diff = millis() - button_time_stamp;

  // ----------------------------------------------------------
  // Short Press → Send Click Event
  // ----------------------------------------------------------

  // If button is released after debounce time
  if (button_state && time_diff > debouceTime && digitalRead(buttonPin) == HIGH) {

    // Mark button as released
    button_state = false;

    Serial.println("User button released. Sending Click event!");

    // Send a click event to Matter Controller
    // This can trigger automations or actions
    SmartButton.click();
  }

  // ----------------------------------------------------------
  // Long Press → Decommission Device
  // ----------------------------------------------------------

  // If button is held for more than 5 seconds
  if (button_state && time_diff > decommissioningTimeout) {

    Serial.println("Decommissioning Generic Switch Matter Accessory.");

    // Remove device from Matter network (factory reset)
    Matter.decommission();

    // Reset timestamp to prevent repeated execution
    button_time_stamp = millis();
  }
}
