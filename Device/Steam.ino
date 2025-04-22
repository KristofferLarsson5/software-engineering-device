#include <ArduinoJson.h>

#define SENSOR_PIN A0                 // Analog pin for steam sensor
const int BUFFER_SIZE = 128;
const unsigned long UPDATE_INTERVAL = 2000; // ms between sensor readings

bool isRegistered = false;
int deviceID = -1;
unsigned long lastUpdate = 0;

void setup() {
  Serial.begin(9600);
  delay(1000);
  registerDevice(); // register to server
}

void loop() {
  // Handle registration response
  if (Serial.available()) {
    char jsonBuffer[BUFFER_SIZE];
    int index = 0;

    while (Serial.available() > 0 && index < BUFFER_SIZE - 1) {
      char c = Serial.read();
      jsonBuffer[index++] = c;
      delay(2);
    }
    jsonBuffer[index] = '\0';

    StaticJsonDocument<BUFFER_SIZE> doc;
    DeserializationError error = deserializeJson(doc, jsonBuffer);

    if (!error) {
      const char* messageType = doc["message_type"];

      if (strcmp(messageType, "registered") == 0 && doc.containsKey("device_id")) {
        deviceID = doc["device_id"];
        isRegistered = true;
        sendAck("registered");
      }
    }
  }

  // Send sensor data every UPDATE_INTERVAL ms
  unsigned long currentMillis = millis();
  if (isRegistered && (currentMillis - lastUpdate >= UPDATE_INTERVAL)) {
    lastUpdate = currentMillis;
    sendSensorData();
  }
}

void registerDevice() {
  StaticJsonDocument<BUFFER_SIZE> doc;
  doc["message_type"] = "register";
  doc["device_type"] = "steam";      // This is the new sensor type
  doc["pin"] = SENSOR_PIN;

  char jsonBuffer[BUFFER_SIZE];
  serializeJson(doc, jsonBuffer);
  Serial.println(jsonBuffer);
}

void sendAck(const char* status) {
  StaticJsonDocument<BUFFER_SIZE> doc;
  doc["message_type"] = "ack";
  if (deviceID != -1) {
    doc["device_id"] = deviceID;
  }
  doc["status"] = status;

  char jsonBuffer[BUFFER_SIZE];
  serializeJson(doc, jsonBuffer);
  Serial.println(jsonBuffer);
}

void sendSensorData() {
  int rawValue = analogRead(SENSOR_PIN);  // Read analog value from sensor

  StaticJsonDocument<BUFFER_SIZE> doc;
  doc["message_type"] = "sensor_data";
  doc["device_type"] = "steam";
  doc["device_id"] = deviceID;
  doc["value"] = rawValue;

  char jsonBuffer[BUFFER_SIZE];
  serializeJson(doc, jsonBuffer);
  Serial.println(jsonBuffer);
}
