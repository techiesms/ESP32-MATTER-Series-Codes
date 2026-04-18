/*************************************************************
  ESP32 MATTER SERIES – EPISODE 04
  Matter Temperature & Humidity Sensor using DHT11

  Board: XIAO ESP32 C6
  IDE  : Arduino IDE 2.3.7
  ESP32 Boards Package: v3.3.6
  DHT Sensoe Library : 1.4.6

  What this episode demonstrates:
  - Creating multiple Matter endpoints
  - Temperature Sensor endpoint
  - Humidity Sensor endpoint
  - Reading environmental data from DHT11
  - Updating Matter attributes from hardware sensor
  - Decommissioning device using BOOT button


  Building and Flashing
  - Select **"Huge APP (3MB No OTA/1MB SPIFFS)"** from **Tools > Partition Scheme** menu.
  - Enable **"Erase All Flash Before Sketch Upload"** option from **Tools** menu.

  Hardware Used:
  - Seeed Studio XIAO ESP32 C6
  - DHT11 Temperature & Humidity Sensor

  Purchase Link [Consider purchasing from techiesms stores to support our free educationla Videos and Codes]:
  XIAO ESP32 C6 - https://techiesms.com/product/seeed-studio-xiao-esp32c6/
  DHT11 Sensor - https://techiesms.com/product/dht11-temperature-and-humidity/
  840pts Breadboard - https://techiesms.com/product/gl-12-840-points-solderless-breadboard/

  YouTube Playlist (ESP32 Matter Series):
  https://www.youtube.com/techiesms

  Author: Sachin Soni
*************************************************************/

// ------------------------------------------------------------
// Matter Core Library
// ------------------------------------------------------------
#include <Matter.h>

// ------------------------------------------------------------
// DHT Sensor Library
// ------------------------------------------------------------
#include <DHT.h>

// ------------------------------------------------------------
// DHT Sensor Configuration
// ------------------------------------------------------------

// Pin connected to DHT sensor data
#define DHTPIN D0

// DHT sensor type
#define DHTTYPE DHT11

// Create DHT sensor object
DHT dht(DHTPIN, DHTTYPE);

// ------------------------------------------------------------
// Matter Endpoints
// ------------------------------------------------------------

// Matter Temperature Sensor endpoint
MatterTemperatureSensor TemperatureSensor;

// Matter Humidity Sensor endpoint
MatterHumiditySensor HumiditySensor;

// ------------------------------------------------------------
// Button Configuration
// ------------------------------------------------------------

// BOOT button used for decommissioning the Matter device
const uint8_t buttonPin = BOOT_PIN;

// Variables used for button press detection
uint32_t button_time_stamp = 0;
bool button_state = false;
const uint32_t decommissioningTimeout = 5000;  // Time required to hold button for device reset


// ------------------------------------------------------------
// Setup Function
// Runs once when the device boots
// ------------------------------------------------------------
void setup() {
  // Configure BOOT button as input with internal pull-up
  pinMode(buttonPin, INPUT_PULLUP);

  // Start Serial Monitor for debugging
  Serial.begin(115200);

  // Initialize DHT sensor
  dht.begin();

  // ----------------------------------------------------------
  // Initialize Matter Endpoints with default values
  // ----------------------------------------------------------

  TemperatureSensor.begin();
  HumiditySensor.begin();

  // ----------------------------------------------------------
  // Start Matter Stack
  // Must be called after all endpoints are initialized
  // ----------------------------------------------------------
  Matter.begin();

  // ----------------------------------------------------------
  // Matter Commissioning Check
  // ----------------------------------------------------------

  if (!Matter.isDeviceCommissioned()) {
    Serial.println("Matter device not commissioned");

    // Display pairing information
    Serial.printf("Manual pairing code: %s\n",
                  Matter.getManualPairingCode().c_str());

    Serial.printf("QR Code URL: %s\n",
                  Matter.getOnboardingQRCodeUrl().c_str());

    // Wait until the device is commissioned
    while (!Matter.isDeviceCommissioned()) {
      Serial.println("Waiting for Matter commissioning...");
      delay(5000);
    }

    Serial.println("Matter commissioned successfully");
  }
}


// ------------------------------------------------------------
// Loop Function
// Runs continuously after setup()
// ------------------------------------------------------------
void loop() {

  // Variable to store last time sensor data was sent
  static uint32_t lastSensorUpdate = 0;

  // ----------------------------------------------------------
  // Read Sensor Data every 5 seconds
  // ----------------------------------------------------------

  if (millis() - lastSensorUpdate >= 5000) {

    // Update the timestamp
    lastSensorUpdate = millis();

    // Read temperature and humidity from DHT sensor
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    // Check if sensor returned valid values
    if (!isnan(temperature) && !isnan(humidity)) {

      Serial.printf("Temp: %.2f C\n", temperature);
      Serial.printf("Humidity: %.2f %%\n", humidity);

      // Update Matter attributes
      TemperatureSensor.setTemperature(temperature);
      HumiditySensor.setHumidity(humidity);

    } else {

      Serial.println("Failed to read from DHT sensor");
    }
  }

  // ----------------------------------------------------------
  // Button Handling for Decommissioning
  // ----------------------------------------------------------

  // Detect button press
  if (digitalRead(buttonPin) == LOW && !button_state) {
    button_time_stamp = millis();
    button_state = true;
  }

  // Detect button release
  if (digitalRead(buttonPin) == HIGH && button_state) {
    button_state = false;
  }

  // Calculate how long the button has been pressed
  uint32_t time_diff = millis() - button_time_stamp;

    // ----------------------------------------------------------
  // Long Press → Decommission Device
  // ----------------------------------------------------------


  // If button is held for more than 5 seconds
  if (button_state && time_diff > decommissioningTimeout) {

    Serial.println("Decommissioning Matter device");

    // Remove device from Matter network
    Matter.decommission();
  }
}