#include <ArduinoJson.h>

// Pin definitions
#define LIGHT_PIN 13
#define FAN_INA_PIN 7
#define FAN_INB_PIN 6

const int BUFFER_SIZE = 128;

// Registration tracking
int registrationStep = 0;
bool lightRegistered = false;
bool fanRegistered = false;

int lightID = -1;
int fanID = -1;

void setup() {
  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(FAN_INA_PIN, OUTPUT);
  pinMode(FAN_INB_PIN, OUTPUT);

  digitalWrite(LIGHT_PIN, LOW);
  stopFan();

  Serial.begin(9600);
  delay(1000);

  // Register devices
  registerDevice("light", LIGHT_PIN);  // D1 - Lamp
  delay(100);
  registerDevice("fan", FAN_INA_PIN);  // D2 - Fan
}

void loop() {
  static char jsonBuffer[BUFFER_SIZE];
  static int index = 0;

  while (Serial.available() > 0) {
    char c = Serial.read();

    if (c == '\n') {
      jsonBuffer[index] = '\0';

      StaticJsonDocument<BUFFER_SIZE> doc;
      DeserializationError error = deserializeJson(doc, jsonBuffer);

      if (!error) {
        const char* messageType = doc["message_type"];

        // Handle registration confirmation
        if (strcmp(messageType, "registered") == 0 && doc.containsKey("device_id")) {
          int assignedID = doc["device_id"];

          if (registrationStep == 0) {
            lightID = assignedID;
            lightRegistered = true;
            sendAck(lightID, "registered");
          } else if (registrationStep == 1) {
            fanID = assignedID;
            fanRegistered = true;
            sendAck(fanID, "registered");
          }

          registrationStep++;
        }

        // Handle device update
        else if (strcmp(messageType, "device_update") == 0 && doc.containsKey("device_id") && doc.containsKey("status")) {
          int incomingId = doc["device_id"];
          const char* status = doc["status"];

          if (incomingId == lightID && lightRegistered) {
            digitalWrite(LIGHT_PIN, strcmp(status, "on") == 0 ? HIGH : LOW);
            sendAck(lightID, status);
          } else if (incomingId == fanID && fanRegistered) {
            if (strcmp(status, "on") == 0) startFan();
            else stopFan();
            sendAck(fanID, status);
          }
        }
      }

      index = 0;
      memset(jsonBuffer, 0, BUFFER_SIZE);
    } else if (index < BUFFER_SIZE - 1) {
      jsonBuffer[index++] = c;
    }
  }
}

// Send device registration
void registerDevice(const char* type, int pin) {
  StaticJsonDocument<BUFFER_SIZE> doc;
  doc["message_type"] = "register";
  doc["device_type"] = type;
  doc["pin"] = pin;

  char jsonBuffer[BUFFER_SIZE];
  serializeJson(doc, jsonBuffer);
  Serial.println(jsonBuffer);
}

// Send ACK back to server
void sendAck(int id, const char* status) {
  StaticJsonDocument<BUFFER_SIZE> doc;
  doc["message_type"] = "ack";
  doc["device_id"] = id;
  doc["status"] = status;

  char jsonBuffer[BUFFER_SIZE];
  serializeJson(doc, jsonBuffer);
  Serial.println(jsonBuffer);
}

// Control fan
void startFan() {
  digitalWrite(FAN_INA_PIN, LOW);
  digitalWrite(FAN_INB_PIN, HIGH);
}

void stopFan() {
  digitalWrite(FAN_INA_PIN, LOW);
  digitalWrite(FAN_INB_PIN, LOW);
}
