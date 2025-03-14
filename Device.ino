#include <ArduinoJson.h>

#define LIGHT_PIN 13  // Adjust this if using another pin
const int BUFFER_SIZE = 128;  // JSON buffer size

bool isRegistered = false;
int deviceID = -1;

void setup() {
    pinMode(LIGHT_PIN, OUTPUT);
    Serial.begin(9600);  // Start serial communication

    // Register the device with the server
    registerDevice();
}

void loop() {
    if (Serial.available()) {
        char jsonBuffer[BUFFER_SIZE];
        int index = 0;

        // Read incoming JSON message from the server
        while (Serial.available()) {
            char c = Serial.read();
            if (index < BUFFER_SIZE - 1) {
                jsonBuffer[index++] = c;
            }
            delay(2);
        }
        jsonBuffer[index] = '\0';

        // Parse JSON
        StaticJsonDocument<BUFFER_SIZE> doc;
        DeserializationError error = deserializeJson(doc, jsonBuffer);

        if (!error) {
            String messageType = doc["message_type"];

            if (messageType == "device_update") { 
                int receivedDeviceID = doc["device_id"];
                String status = doc["status"];

                if (receivedDeviceID == deviceID || deviceID == -1) {  // Check if correct device
                    if (status == "on") {
                        digitalWrite(LIGHT_PIN, HIGH);
                        sendAck(receivedDeviceID, "on");
                    }
                    else if (status == "off") {
                        digitalWrite(LIGHT_PIN, LOW);
                        sendAck(receivedDeviceID, "off");
                    }
                }
            }
            else if (messageType == "registered") {
                if (doc.containsKey("device_id")) {
                    deviceID = doc["device_id"];
                    isRegistered = true;
                    sendAck(deviceID, "registered");  // Sending registration acknowledgment
                }
            }
        }
    }
}

// Function to register the device with the server
void registerDevice() {
    StaticJsonDocument<BUFFER_SIZE> doc;
    doc["message_type"] = "register";
    doc["device_type"] = "light";

    char jsonBuffer[BUFFER_SIZE];
    serializeJson(doc, jsonBuffer);
    Serial.println(jsonBuffer);  // Send registration request to the server
}

// Function to send acknowledgment back to the server
void sendAck(int deviceID, const char* status) {
    StaticJsonDocument<BUFFER_SIZE> doc;
    doc["message_type"] = "ack";
    doc["device_id"] = deviceID;
    doc["status"] = status;

    char jsonBuffer[BUFFER_SIZE];
    serializeJson(doc, jsonBuffer);
    Serial.println(jsonBuffer);  // Sends acknowledgment in JSON format
}

