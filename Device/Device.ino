#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Fan motor pins
#define FAN_INA_PIN 7
#define FAN_INB_PIN 6

// Sensor pins
#define STEAM_SENSOR_PIN A2
#define HUMIDITY_SENSOR_PIN A3
#define LIGHT_SENSOR_PIN A1
#define MOTION_SENSOR_PIN 12
#define GAS_SENSOR_PIN A0
#define BUTTON_SENSOR_PIN1 4
#define BUTTON_SENSOR_PIN2 8


const int BUFFER_SIZE = 256;
const unsigned long TEMP_SEND_INTERVAL = 10000;

int sensorIdCounter = 1000;

// Device struct and array
struct Device {
  const char* type;
  int pin;
  int id;          // for devices
  int sensor_id;   // for sensors
  bool registered;
};

#define MAX_DEVICES 12
Device devices[MAX_DEVICES] = {
  {"light", 13, -1, -1, false},
  {"light", 5, -1, -1, false},
  {"fan", FAN_INA_PIN, -1, -1, false},
  {"screen", 0, -1, -1, false},
  {"buzzer", 3, -1, -1, false},
  {"steam_sensor", STEAM_SENSOR_PIN, -1, -1, false},
  {"humidity_sensor", HUMIDITY_SENSOR_PIN, -1, -1, false},
  {"light_sensor", LIGHT_SENSOR_PIN, -1, -1, false},
  {"motion_sensor", MOTION_SENSOR_PIN, -1, -1, false},
  {"gas_sensor", GAS_SENSOR_PIN, -1, -1, false},
  {"button_sensor1", BUTTON_SENSOR_PIN1, -1, -1, false},
  {"button_sensor2", BUTTON_SENSOR_PIN2, -1, -1, false},
};

int deviceCount = 12;
int currentDeviceIndex = 0;
bool registrationInProgress = false;

unsigned long lastTempSend = 0;

void setup() {
  Serial.begin(9600);
  delay(1000);

  assignSensorIds();

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

// Assign dynamic sensor IDs
void assignSensorIds() {
  for (int i = 0; i < deviceCount; i++) {
    if (strstr(devices[i].type, "sensor") != nullptr) {
      devices[i].sensor_id = ++sensorIdCounter;
    }
  }
}

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

        // Handle both device_id and sensor_id
        if (strcmp(messageType, "registered") == 0) {
          if (doc.containsKey("device_id")) {
            int id = doc["device_id"];
            devices[currentDeviceIndex].id = id;
            devices[currentDeviceIndex].registered = true;
            sendAck(id, "registered");
          } else if (doc.containsKey("sensor_id")) {
            int id = doc["sensor_id"];
            devices[currentDeviceIndex].sensor_id = id;
            devices[currentDeviceIndex].registered = true;
            sendAck(id, "registered");
          }

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
              } else if (strcmp(type, "buzzer") == 0) {
                if (strcmp(status, "on") == 0) tone(devices[i].pin, 1000);
                else noTone(devices[i].pin);
              } else if (strcmp(type, "fan") == 0) {
                if (strcmp(status, "on") == 0) startFan();
                else stopFan();
              } else if (strcmp(type, "screen") == 0) {
                lcd.clear();
                if (strcmp(status, "on") == 0) {
                  lcd.setCursor(0, 0);
                  lcd.print("Welcome Home");
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


// Track previous button states
static int lastButton1State = 0;
static int lastButton2State = 0;
  // Sensor updates
for (int i = 0; i < deviceCount; i++) {
  if (!devices[i].registered) continue;

  if (strcmp(devices[i].type, "steam_sensor") == 0) {
    float tempC = readTemperature();
    sendSensorJson(devices[i].sensor_id, "steam", round(tempC), "celsius", "steam1");

  } else if (strcmp(devices[i].type, "humidity_sensor") == 0) {
    float humidity = readHumidity();
    sendSensorJson(devices[i].sensor_id, "humidity", round(humidity), "%", "humid1");

  } else if (strcmp(devices[i].type, "light_sensor") == 0) {
    float light = readLightLevel();
    sendSensorJson(devices[i].sensor_id, "light", round(light), "bool", "light1");

  } else if (strcmp(devices[i].type, "motion_sensor") == 0) {
    float motion = readMotion();
    sendSensorJson(devices[i].sensor_id, "motion", round(motion), "boolean", "motion1");

  } else if (strcmp(devices[i].type, "gas_sensor") == 0) {
    float gas = readGas();
    sendSensorJson(devices[i].sensor_id, "gas", round(gas), "ppm", "gas1");

  } else if (strcmp(devices[i].type, "button_sensor1") == 0) {
    int button1State = digitalRead(devices[i].pin);
    sendSensorJson(devices[i].sensor_id, "button", button1State, "boolean", "button_sensor1");

    if (button1State == HIGH && lastButton1State == LOW) {
      startFan();
    }
    lastButton1State = button1State;

  } else if (strcmp(devices[i].type, "button_sensor2") == 0) {
    int button2State = digitalRead(devices[i].pin);
    sendSensorJson(devices[i].sensor_id, "button", button2State, "boolean", "button_sensor2");

    if (button2State == HIGH && lastButton2State == LOW) {
      stopFan();
    }
    lastButton2State = button2State;
  }
}

  if (!registrationInProgress && currentDeviceIndex < deviceCount) {
    delay(100);
    registerNextDevice();
  }
}

// Register device or sensor
void registerNextDevice() {
  if (currentDeviceIndex >= deviceCount) return;

  registrationInProgress = true;
  registerDevice(devices[currentDeviceIndex].type, devices[currentDeviceIndex].pin);
}

void registerDevice(const char* type, int pin) {
  StaticJsonDocument<BUFFER_SIZE> doc;

  if (strstr(type, "sensor") != nullptr) {
    doc["message_type"] = "register";
    doc["sensor_type"] = type;
    doc["pin"] = pin;
  } else {
    doc["message_type"] = "register";
    doc["device_type"] = type;
    doc["pin"] = pin;
  }

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

// Sensor reads
float readTemperature() {
  int analogValue = analogRead(STEAM_SENSOR_PIN);
  float voltage = analogValue * (5.0 / 1023.0);

  // 🔍 Print like the debug block
  Serial.print("[Steam Sensor] Raw: ");
  Serial.print(analogValue);
  Serial.print(" | Voltage: ");
  Serial.print(voltage, 2);
  Serial.println(" V");

  return voltage;  // Return voltage directly for now
}

float readHumidity() {
  int analogValue = analogRead(HUMIDITY_SENSOR_PIN);
  float voltage = analogValue * (5.0 / 1023.0);
  float humidityPercent = voltage * 100.0 / 5.0;
  return humidityPercent;
}

float readLightLevel() {
  int analogValue = analogRead(LIGHT_SENSOR_PIN);
  float voltage = analogValue * (5.0 / 1023.0);
  float lightPercent = (voltage / 5.0) * 100.0;
  return lightPercent;
}

// Read motion sensor (digital sensor like PIR)
float readMotion() {
  return digitalRead(MOTION_SENSOR_PIN);
}

// Read gas sensor (analog sensor like MQ-x)
float readGas() {
  int analogValue = analogRead(GAS_SENSOR_PIN);
  float voltage = analogValue * (5.0 / 1023.0);
  float gasPercent = (voltage / 5.0) * 100.0;
  return gasPercent;
}


// Send sensor data
void sendSensorJson(int id, const char* type, float value, const char* unit, const char* name) {
  StaticJsonDocument<BUFFER_SIZE> doc;
  doc["message_type"] = "sensor_data";
  doc["sensor_id"] = id;
  doc["sensor_type"] = type;
  doc["value"] = value;
  doc["unit"] = unit;
  doc["sensor_name"] = name;
  doc["location"] = "loc";
  doc["registered"] = true;

  char buffer[BUFFER_SIZE];
  if (serializeJson(doc, buffer) > 0) {
    Serial.println(buffer);
  }
}
