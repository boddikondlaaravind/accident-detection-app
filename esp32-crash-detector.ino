/*
 * RapidAid — ESP32 Crash / Panic Button Trigger
 * ───────────────────────────────────────────────
 * Sends an SOS trigger to your relay server when:
 *   (a) a manual panic button is pressed, OR
 *   (b) an MPU6050 accelerometer detects a sudden hard impact
 *
 * HARDWARE
 *   - ESP32 dev board
 *   - MPU6050 accelerometer/gyro module (optional, for auto crash
 *     detection — wire SDA→GPIO21, SCL→GPIO22, VCC→3.3V, GND→GND)
 *   - Push button between GPIO4 and GND (optional, for manual SOS)
 *
 * LIBRARIES (install via Arduino Library Manager)
 *   - "Adafruit MPU6050" (and its dependency "Adafruit Unified Sensor")
 *
 * SETUP
 *   1. Fill in WIFI_SSID / WIFI_PASSWORD below.
 *   2. Fill in RELAY_URL with your deployed relay server's address.
 *   3. Fill in DEVICE_ID / DEVICE_KEY — copy these from the RapidAid
 *      webpage: Profile → 📡 IoT Device → Copy Device Link
 *      (the link looks like ?device_trigger=<DEVICE_ID>&key=<DEVICE_KEY>)
 *   4. Flash to your ESP32. Open Serial Monitor at 115200 baud to watch it.
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// ── CONFIG — fill these in ──────────────────────────────────
const char* WIFI_SSID     = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* RELAY_URL     = "https://your-relay.onrender.com"; // no trailing slash
const char* DEVICE_ID     = "PASTE_USER_ID_HERE";
const char* DEVICE_KEY    = "PASTE_DEVICE_KEY_HERE";

// Impact threshold in g's — tune this. A hard crash is typically >4-6g.
// Start high and lower it during testing so you don't get false alarms.
const float IMPACT_THRESHOLD_G = 5.0;

const int BUTTON_PIN = 4; // manual panic button, active LOW (button → GND)

Adafruit_MPU6050 mpu;
bool mpuReady = false;
unsigned long lastTriggerAt = 0;
const unsigned long TRIGGER_COOLDOWN_MS = 30000; // don't spam-trigger

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  connectWiFi();

  Wire.begin();
  if (mpu.begin()) {
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    mpuReady = true;
    Serial.println("MPU6050 ready — crash detection active.");
  } else {
    Serial.println("MPU6050 not found — only the manual button will work.");
  }
}

void loop() {
  // Manual panic button
  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("Panic button pressed!");
    sendTrigger();
    delay(1000); // simple debounce
  }

  // Automatic crash detection
  if (mpuReady) {
    sensors_event_t accel, gyro, temp;
    mpu.getEvent(&accel, &gyro, &temp);

    float gForce = sqrt(
      accel.acceleration.x * accel.acceleration.x +
      accel.acceleration.y * accel.acceleration.y +
      accel.acceleration.z * accel.acceleration.z
    ) / 9.81;

    if (gForce > IMPACT_THRESHOLD_G) {
      Serial.printf("Impact detected: %.2fg\n", gForce);
      sendTrigger();
    }
  }

  delay(100);
}

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected: " + WiFi.localIP().toString());
}

void sendTrigger() {
  unsigned long now = millis();
  if (now - lastTriggerAt < TRIGGER_COOLDOWN_MS) return; // avoid spamming
  lastTriggerAt = now;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected — cannot send trigger.");
    return;
  }

  HTTPClient http;
  String url = String(RELAY_URL) + "/trigger?device=" + DEVICE_ID + "&key=" + DEVICE_KEY;
  http.begin(url);
  int code = http.POST("");
  Serial.printf("Trigger sent — server responded %d\n", code);
  http.end();
}
