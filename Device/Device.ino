#include <ArduinoJson.h>

#define LIGHT_PIN 13                 // Pin controlling the light
const int BUFFER_SIZE = 128;        // Buffer size for JSON handling

bool isRegistered = false;
int deviceID = -1;

void setup() {
  pinMode(LIGHT_PIN, OUTPUT);
  digitalWrite(LIGHT_PIN, LOW);     // Ensure light is off on start
  Serial.begin(9600);
  delay(1000);                      // Wait for Serial to be ready
  registerDevice();                 // Send initial registration message
}

void loop() {
  if (Serial.available()) {
    char jsonBuffer[BUFFER_SIZE];
    int index = 0;

    while (Serial.available() > 0 && index < BUFFER_SIZE - 1) {
      char c = Serial.read();
      jsonBuffer[index++] = c;
      delay(2);
    }
    jsonBuffer[index] = '\0'; // Null-terminate

    StaticJsonDocument<BUFFER_SIZE> doc;
    DeserializationError error = deserializeJson(doc, jsonBuffer);

    if (!error) {
      const char* messageType = doc["message_type"];

      // Handle "registered" message from server
      if (strcmp(messageType, "registered") == 0 && doc.containsKey("device_id")) {
        deviceID = doc["device_id"];
        isRegistered = true;
        sendAck("registered"); 
      }

      // Handle "device_update" command (turn light on/off)
      else if (strcmp(messageType, "device_update") == 0 && doc.containsKey("device_id")) {
        int incomingId = doc["device_id"];
        const char* status = doc["status"];

        if (incomingId == deviceID && isRegistered) {
          if (strcmp(status, "on") == 0) {
            digitalWrite(LIGHT_PIN, HIGH);
            sendAck("on");
          } else if (strcmp(status, "off") == 0) {
            digitalWrite(LIGHT_PIN, LOW);
            sendAck("off");
          }
        }
      }
    }
  }
}

// Function to register this device with the server
void registerDevice() {
  StaticJsonDocument<BUFFER_SIZE> doc;
  doc["message_type"] = "register";
  doc["device_type"] = "light";
  doc["pin"] = LIGHT_PIN;

  char jsonBuffer[BUFFER_SIZE];
  serializeJson(doc, jsonBuffer);
  Serial.println(jsonBuffer);       // Send JSON to server
}

// Function to send ACK message to server
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
