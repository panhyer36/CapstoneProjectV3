#include <Wire.h>
#include <DHT.h>
#include <SoftwareSerial.h>
#include <SparkFun_SCD4x_Arduino_Library.h>
#include <PMS.h>

// ----- Sensor and Pin Configuration -----
// DHT22 setup: connected to digital pin 2
#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// PMS5003 setup: TX to digital pin 3, RX to digital pin 4 (using SoftwareSerial)
SoftwareSerial pmsSerial(4, 3);
PMS pms(pmsSerial);
PMS::DATA pmsData;  // Variable to store PMS5003 data

// SCD41 setup: using I2C (A4 = SDA, A5 = SCL)
SCD4x scd41;

// ----- Time-division Control Parameters -----
unsigned long cycleStart = 0;
const unsigned long cycleDuration = 10000;  // Total cycle duration: 10 seconds

// Flags for time-division sensor reading, to avoid duplicate reads in one cycle
bool co2ReadFlag = false;   // Flag for reading SCD41 (CO₂)
bool pmsReadFlag = false;   // Flag for reading PMS5003
bool dhtReadFlag = false;   // Flag for reading DHT22

// Temporary variables to store sensor values
uint16_t co2Value = 0;
float dhtTemperature = 0;
float dhtHumidityVal = 0;

void setup() {
  delay(10000);
  Serial.begin(9600);
  while (!Serial); // Wait for Serial connection (especially when using USB)
  // Initialize DHT22
  dht.begin();

  // Initialize PMS5003
  pmsSerial.begin(9600);

  // Initialize I2C and SCD41
  Wire.begin();
  if (!scd41.begin()) {
    Serial.println("SCD41 sensor not detected. Please check the wiring!");
    while (1);
  }
  // Start periodic measurement for SCD41
  if (!scd41.startPeriodicMeasurement()) {
    Serial.println("Failed to start measurement on SCD41!");
    while (1);
  }
  cycleStart = millis();
  delay(10000);
}

void loop() {
  unsigned long elapsed = millis() - cycleStart;

  // At 3rd second: Read SCD41 CO₂ value (usually non-blocking)
  if (!co2ReadFlag && elapsed >= 3000) {
    co2Value = scd41.getCO2();
    co2ReadFlag = true;
  }

  // At 6th second: Read PMS5003 PM values (with timeout protection, max 500ms wait)
  if (!pmsReadFlag && elapsed >= 6000) {
    unsigned long start = millis();
    bool pmsSuccess = false;
    while (millis() - start < 2500) {  // Max wait 500ms
      if (pms.read(pmsData)) {        // If a full data frame is received
        pmsSuccess = true;
        break;
      }
    }
    pmsReadFlag = true; // Mark as read regardless of success/failure
  }

  // At 9th second: Read DHT22 temperature and humidity
  if (!dhtReadFlag && elapsed >= 9000) {
    dhtTemperature = dht.readTemperature();
    dhtHumidityVal = dht.readHumidity();
    dhtReadFlag = true;
  }

  // At 10th second: Output all sensor results and reset cycle
  if (elapsed >= cycleDuration) {
    Serial.println("{");

    // Output CO₂ value
    Serial.print("\"CO2 (ppm)\": ");
    Serial.print(co2Value);
    Serial.println(",");

    // Output PMS5003 values
    Serial.print("\"PM1.0 (ug/m3)\": ");
    Serial.print(pmsData.PM_AE_UG_1_0);
    Serial.println(",");

    Serial.print("\"PM2.5 (ug/m3)\": ");
    Serial.print(pmsData.PM_AE_UG_2_5);
    Serial.println(",");

    Serial.print("\"PM10.0 (ug/m3)\": ");
    Serial.print(pmsData.PM_AE_UG_10_0);
    Serial.println(",");

    // Output DHT22 values
    Serial.print("\"Temperature (C)\": ");
    Serial.print(dhtTemperature);
    Serial.println(",");

    Serial.print("\"Humidity (%)\": ");
    Serial.println(dhtHumidityVal);

    Serial.println("}");
    Serial.println();

    // Reset cycle and all flags
    cycleStart = millis();
    co2ReadFlag = false;
    pmsReadFlag = false;
    dhtReadFlag = false;
  }
}