#include <ArduinoJson.h>

#define LIGHT_PIN 13  // Adjust this if using another pin
const int BUFFER_SIZE = 128;  // JSON buffer size

bool isRegistered = false;
int moduleID = -1;

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

            if (messageType == "lamp_control") {
                int receivedModuleID = doc["module_id"];
                String status = doc["status"];

                if (receivedModuleID == moduleID || moduleID == -1) {  // Check if correct device
                    if (status == "on") {
                        digitalWrite(LIGHT_PIN, HIGH);
                        sendAck(receivedModuleID, "on");
                    } 
                    else if (status == "off") {
                        digitalWrite(LIGHT_PIN, LOW);
                        sendAck(receivedModuleID, "off");
                    }
                }
            }
            else if (messageType == "registered") {
                // Store module ID from the server
                if (doc.containsKey("module_id")) {
                    moduleID = doc["module_id"];
                    isRegistered = true;
                    Serial.println("Module registered successfully!");
                }
            }
        }
    }
}
NYTT
[16:09]
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
void sendAck(int moduleID, const char* status) {
    StaticJsonDocument<BUFFER_SIZE> doc;
    doc["message_type"] = "ack";
    doc["module_id"] = moduleID;
    doc["status"] = status;

    char jsonBuffer[BUFFER_SIZE];
    serializeJson(doc, jsonBuffer);
    Serial.println(jsonBuffer);
}

