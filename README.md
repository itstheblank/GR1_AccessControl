# IoT Access Control Node via Ethernet & MQTT

![ESP32](https://img.shields.io/badge/ESP32-WT32--ETH01-blue.svg)
![Framework](https://img.shields.io/badge/Framework-Arduino-00979D.svg)
![Protocol](https://img.shields.io/badge/Protocol-MQTT-660066.svg)

## Project Overview

This project implements a robust, real-time Access Control Node using the **WT32-ETH01** (ESP32 with built-in Ethernet LAN8720). By utilizing a wired LAN connection and the lightweight MQTT protocol, the system ensures high stability and minimal latency compared to traditional Wi-Fi solutions.

The node is capable of reading RFID tags, remotely actuating an electromagnetic lock (Relay), and accurately monitoring the physical Open/Closed state of the door via an Opto-isolator.

## Key Features

* **Wired Ethernet Stability:** Utilizes LAN8720 PHY for uninterrupted connection to the local network and MQTT Broker.
* **Non-blocking Architecture:** Employs `millis()` for timer management (3-second auto-lock and 1-second RFID cooldown), ensuring the main loop runs continuously without freezing.
* **Real-time State Monitoring:** Uses a PC817 Opto-isolator with Active-Low logic (`INPUT_PULLUP`) and software debouncing to accurately detect and report the physical door status.
* **Galvanic Isolation:** Separates high-current mechanical components (Relay) from the MCU to prevent brownouts and signal noise.

## Hardware Requirements

* **Microcontroller:** WT32-ETH01 (ESP32)
* **RFID Reader:** MFRC522 (13.56MHz)
* **Actuator:** 5V 1-Channel Relay Module (for lock)
* **Sensor:** PC817 4-Channel Opto-isolator Module (for door status)
* **Other:** USB to TTL CH340 (for programming), External 5V Power Supply.

## Pin Mapping (WT32-ETH01)

| Component       | Pin Function | WT32-ETH01 Pin | Note                                       |
| :-------------- | :----------- | :------------- | :----------------------------------------- |
| **RC522** | RST          | `IO2`        |                                            |
| **RC522** | SS (SDA)     | `IO4`        |                                            |
| **RC522** | MOSI         | `IO12`       |                                            |
| **RC522** | MISO         | `IO35`       |                                            |
| **RC522** | SCK          | `IO14`       |                                            |
| **Relay** | IN           | `IO15`       | Active Low / High depending on module      |
| **Opto**  | Signal       | `IO32` (CFG) | Configured as`INPUT_PULLUP` (Active Low) |

## MQTT Topics & Payloads

The system communicates with the central server (e.g., EMQX) via the following topics:

* **Subscribe (Receive commands):**

  * Topic: `gr1/esp32/control`
  * Payload to open door: `open_door`
* **Publish (Send data):**

  * Topic: `gr1/esp32/rfid`
  * Payload format: `{"uid": "C44E0307"}`
  * Topic: `gr1/esp32/door_status`
  * Payload format: `{"action": "door_opened"}` / `{"status": "open"}` / `{"status": "closed"}`

## Installation & Setup (VS Code / PlatformIO)

1. **Clone this repository:**

   ```bash
   git clone https://github.com/yourusername/access-control-node.git
   cd access-control-node
   ```
2. **Open the project in VS Code:**
   Ensure you have the [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode) installed. Open the project folder in VS Code.
3. **Configure the Project (`platformio.ini`):**
   The project is pre-configured for the `wt32-eth01` board and uses the following libraries:

   * `knolleary/PubSubClient`
   * `miguelbalboa/MFRC522`

   Dependencies will be automatically downloaded by PlatformIO when you build the project.
4. **Network & MQTT Configuration:**
   Before uploading, open `src/main.cpp` (or your config file) and update the broker settings:

   ```cpp
   const char* mqtt_server = "YOUR_MQTT_BROKER_IP";
   const int mqtt_port = 1883;
   ```
5. **Build and Upload:**

   * Connect your WT32-ETH01 via a USB-to-TTL adapter (remember to tie `IO0` to `GND` before powering on to enter download mode).
   * Click the **PlatformIO: Build** (`✓`) button in the bottom status bar.
   * Click the **PlatformIO: Upload** (`→`) button to flash the code.
   * After uploading, disconnect `IO0` from `GND` and reset the board.
6. **Monitor Serial Output:**

   * Click the **PlatformIO: Serial Monitor** (`🔌`) button to view the logs at a baud rate of `115200`.

## Author

**Tran Sy Nguyen**

* Student ID: 20235985
* Class: ICT-01 K68
* Hanoi University of Science and Technology
