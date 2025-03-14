# Smart Home Device - DA330A VT25 Software Engineering  

This repository contains the **device-side implementation** for the **Smart Home project** in the course **DA330A VT25 Software Engineering**. The device interacts with a **physical smart home model** and communicates with a server to control various components.  

## Project Overview  
- This is the **device-side** of the project, running on an **Arduino-based microcontroller**.  
- The system allows remote control of a **lamp** (ON/OFF) through a **server**.  
- The device communicates using **serial communication (USB)** and follows a **JSON-based message structure**.  

## Features  
- **Device Registration**: The device registers itself with the server and receives a unique `device_id`.  
- **Remote Lamp Control**: The server can send `device_update` messages to turn the lamp ON/OFF.  
- **Acknowledgment Messages**: The device sends `ack` responses back to the server for reliable communication.  

## How to Use  
1. **Upload the Arduino Code** to your microcontroller using **Arduino IDE**.  
2. **Run the Server**.  
3. **Send Commands** from the server to control the lamp.  
4. **Monitor Serial Output** to see registration, updates, and acknowledgments.  

