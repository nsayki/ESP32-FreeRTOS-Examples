# 🕹️ ESP32-C6 FreeRTOS Joystick Driver (ADC OneShot & Queues)

This project demonstrates a professional driver implementation for reading and processing an analog Joystick module on the **ESP32-C6** using **FreeRTOS**.

The project is designed using the **Producer-Consumer** architecture, utilizing **ADC OneShot Mode**, **GPIO Polling**, and **FreeRTOS Queues**.

## 🚀 Features
* **ADC OneShot:** Utilizes the new ESP-IDF v5.x compatible ADC driver to read Joystick X and Y axes.
* **Struct over Queue:** X, Y, and Button data are packed into a `struct` and safely transferred between Tasks.
* **Ghosting Fix:** Optimized terminal output that eliminates flickering and residual characters.
* **Deadzone Logic:** Tolerance ranges (deadzones) are defined for joystick sensitivity to prevent drift.

## 🛠️ Wiring Connections

This project is configured for the **ESP32-C6** (DevKit) and a standard **KY-023 Joystick Module**.

| Joystick Pin | ESP32-C6 Pin | Description |
| :--- | :--- | :--- |
| **VCC** | 3.3V / 5V | Power Supply |
| **GND** | GND | Ground |
| **VRx** | **GPIO 3** | Analog X Axis (ADC1 Channel 3) |
| **VRy** | **GPIO 2** | Analog Y Axis (ADC1 Channel 2) |
| **SW** | **GPIO 4** | Button (Switch) - Active Low |

*(Note: The X and Y axes are calibrated in the software based on the orientation of the module. If the directions are inverted, you can adjust them in the code or swap the wiring.)*

## 📂 Software Architecture

The system consists of two main Tasks:

1.  **ADC Reader Task (Producer):**
    * Reads sensor data (X, Y, Button) every 100ms.
    * Packs the data and sends it to the `xJoystickQueue`.
    
2.  **Controller Task (Consumer):**
    * Receives data from the queue.
    * Determines the direction (RIGHT, LEFT, UP, DOWN) based on defined threshold values.
    * Prints the result cleanly to the terminal using ANSI escape codes.

## 💻 How to Run?

To build and flash the project to the board:

```bash
idf.py build
idf.py flash
idf.py monitor