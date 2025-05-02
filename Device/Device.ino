#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Fan motor pins
#define FAN_INA_PIN 7
#define FAN_INB_PIN 6

// Sensor pins
#define TEMP_SENSOR_PIN A0
#define HUMIDITY_SENSOR_PIN A3

const int BUFFER_SIZE = 256;
const unsigned long TEMP_SEND_INTERVAL = 10000;

// Device struct and array
struct Device {
  const char* type;
  int pin;
  int id;
  bool registered;
};

#define MAX_DEVICES 10
Device devices[MAX_DEVICES] = {
  {"light_1", 13, -1, false},
  {"light_2", 5, -1, false},
  {"fan", FAN_INA_PIN, -1, false},
  {"screen", 0, -1, false},
  {"buzzer", 3, -1, false}
};
int deviceCount = 5;
int currentDeviceIndex = 0;
bool registrationInProgress = false;

unsigned long lastTempSend = 0;

// Setup
void setup() {
  Serial.begin(9600);
  delay(1000);

  for (int i = 0; i < deviceCount; i++) {
    if (strcmp(devices[i].type, "fan") == 0) {
      pinMode(FAN_INA_PIN, OUTPUT);
      pinMode(FAN_INB_PIN, OUTPUT);
      stopFan();
    } else if (strcmp(devices[i].type, "screen") != 0) {
      pinMode(devices[i].pin, OUTPUT);
      digitalWrite(devices[i].pin, LOW);
    }
  }

  lcd.init();
  lcd.backlight();
  lcd.clear();

  registerNextDevice();
}

// Main loop
void loop() {
  static char jsonBuffer[BUFFER_SIZE];
  static int index = 0;

  while (Serial.available() > 0) {
    char c = Serial.read();

    if (c == '\n') {
      jsonBuffer[index] = '\0';

      if (strlen(jsonBuffer) < 2) {
        index = 0;
        return;
      }

      StaticJsonDocument<BUFFER_SIZE> doc;
      DeserializationError error = deserializeJson(doc, jsonBuffer);

      if (!error) {
        const char* messageType = doc["message_type"];

        // Registration response
        if (strcmp(messageType, "registered") == 0 && doc.containsKey("device_id")) {
          int id = doc["device_id"];
          devices[currentDeviceIndex].id = id;
          devices[currentDeviceIndex].registered = true;

          delay(100);
          sendAck(id, "registered");
          delay(100);

          currentDeviceIndex++;
          registrationInProgress = false;
        }

        // Handle device updates
        else if (strcmp(messageType, "device_update") == 0 && doc.containsKey("device_id") && doc.containsKey("status")) {
          int incomingId = doc["device_id"];
          const char* status = doc["status"];

          for (int i = 0; i < deviceCount; i++) {
            if (devices[i].registered && devices[i].id == incomingId) {
              const char* type = devices[i].type;

              if (strncmp(type, "light", 5) == 0) {
                digitalWrite(devices[i].pin, strcmp(status, "on") == 0 ? HIGH : LOW);
              }
              else if (strcmp(type, "buzzer") == 0) {
                if (strcmp(status, "on") == 0) {
                  tone(devices[i].pin, 1000); // 1kHz tone
                } else {
                  noTone(devices[i].pin);
                }
              }
              else if (strcmp(type, "fan") == 0) {
                if (strcmp(status, "on") == 0) startFan();
                else stopFan();
              }
              else if (strcmp(type, "screen") == 0) {
                if (strcmp(status, "on") == 0) {
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("Welcome Home");
                } else {
                  lcd.clear();
                }
              }

              sendAck(devices[i].id, status);
              break;
            }
          }
        }
      }

      index = 0;
      memset(jsonBuffer, 0, BUFFER_SIZE);
    } else if (index < BUFFER_SIZE - 1) {
      jsonBuffer[index++] = c;
    }
  }

  // Sensor updates every 10 seconds
  unsigned long currentMillis = millis();
  if (currentMillis - lastTempSend >= TEMP_SEND_INTERVAL) {
    lastTempSend = currentMillis;

    float tempC = readTemperature();
    sendSensorJson(1001, "temperature", round(tempC), "celsius", "temp1");

    float humidity = readHumidity();
    sendSensorJson(1002, "humidity", round(humidity), "%", "humid1");
  }

  // Register next device if ready
  if (!registrationInProgress && currentDeviceIndex < deviceCount) {
    delay(100);
    registerNextDevice();
  }
}

// Register device
void registerNextDevice() {
  if (currentDeviceIndex >= deviceCount) return;

  registrationInProgress = true;
  registerDevice(devices[currentDeviceIndex].type, devices[currentDeviceIndex].pin);
}

void registerDevice(const char* type, int pin) {
  StaticJsonDocument<BUFFER_SIZE> doc;
  doc["message_type"] = "register";
  doc["device_type"] = type;
  doc["pin"] = pin;

  char buffer[BUFFER_SIZE];
  if (serializeJson(doc, buffer) > 0) {
    Serial.println(buffer);
  }
}

// Acknowledge
void sendAck(int id, const char* status) {
  StaticJsonDocument<BUFFER_SIZE> doc;
  doc["message_type"] = "ack";
  doc["device_id"] = id;
  doc["status"] = status;

  char buffer[BUFFER_SIZE];
  if (serializeJson(doc, buffer) > 0) {
    Serial.println(buffer);
  }
}

// Fan control
void startFan() {
  digitalWrite(FAN_INA_PIN, LOW);
  digitalWrite(FAN_INB_PIN, HIGH);
}

void stopFan() {
  digitalWrite(FAN_INA_PIN, LOW);
  digitalWrite(FAN_INB_PIN, LOW);
}

// Read temperature sensor
float readTemperature() {
  int analogValue = analogRead(TEMP_SENSOR_PIN);
  float voltage = analogValue * (5.0 / 1023.0);
  float temperatureC = (voltage - 0.5) * 100.0;
  return temperatureC;
}

// Read humidity sensor
float readHumidity() {
  int analogValue = analogRead(HUMIDITY_SENSOR_PIN);
  float voltage = analogValue * (5.0 / 1023.0);
  float humidityPercent = voltage * 100.0 / 5.0; // Scale 0–5V to 0–100%
  return humidityPercent;
}

// Send sensor data
void sendSensorJson(int id, const char* type, float value, const char* unit, const char* name) {
  StaticJsonDocument<BUFFER_SIZE> doc;
  doc["sensor_id"] = id;
  doc["sensor_type"] = type;
  doc["value"] = value;
  doc["unit"] = unit;
  doc["sensor_name"] = name;
  doc["location"] = "loc";
  doc["registered"] = false;

  char buffer[BUFFER_SIZE];
  if (serializeJson(doc, buffer) > 0) {
    Serial.println(buffer);
  }
}

