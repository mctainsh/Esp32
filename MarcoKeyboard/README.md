# ESP32-S2 Mini Keypad to Keyboard Emulator 

![Keypad](https://github.com/mctainsh/Esp32/blob/main/MarcoKeyboard/Photos/Finished.jpg?raw=true)

This project turns an ESP32-S2 Mini microcontroller connected to a 4x3 Matrix Keypad into a fully functioning USB keyboard emulator. It allows the user to press key combinations (e.g., `*1`) on the keypad, which the ESP32 processes and sends predefined key sequences to a connected computer via USB.


## Table of Contents 
 
- [Project Overview](#project-overview)
 
- [Hardware](#hardware)  
  - [Components](#components)
 
  - [Wiring Diagram](#wiring-diagram)
 
- [Software](#software)  
  - [Features](#features)
 
  - [Key Mappings](#key-mappings)
 
  - [Setup & Installation](#setup--installation)
 
  - [Usage](#usage)
 
- [License](#license)

## Project Overview 

This project enables an ESP32-S2 Mini to act as a keyboard by interpreting key presses from a 4x3 Matrix Keypad. Each keypress or combination of keypresses can trigger a corresponding sequence of keystrokes on the connected computer. The software is written in C++ and is compiled using the Arduino IDE.

### Use Cases 

- Creating custom macros to improve productivity

- Controlling applications or games using simple key combinations

- Automating repetitive tasks with a single button press

## Hardware 

### Components 
 
1. **ESP32-S2 Mini**  - The microcontroller that processes key inputs and sends USB keyboard signals. I got it from AliExpress for about $3.00 [Not affiliate link](https://www.aliexpress.com/item/1005005024560671.html)
 
2. **4x3 Matrix Keypad**  - The input device with 12 buttons (numbered 0-9, `*`, and `#`). I got this for AliExpress for $4.00. [Not an affiliate link](https://www.aliexpress.com/item/4001135475068.html)
 
3. **USB Cable**  - Connects the ESP32 to the computer for both power and communication.
 
4. **Jumper Wires**  - For connecting the keypad to the ESP32.

### Wiring Diagram 
| Keypad Pin | ESP32-S2 Pin | 
| --- | --- | 
| R1 | GPIO 4 | 
| R2 | GPIO 5 | 
| R3 | GPIO 6 | 
| R4 | GPIO 7 | 
| C1 | GPIO 8 | 
| C2 | GPIO 9 | 
| C3 | GPIO 10 | 
 
- **R1 - R4** : Rows of the matrix keypad
 
- **C1 - C3** : Columns of the matrix keypad

## Software 

### Features 

- Emulates a USB keyboard using the ESP32-S2 Mini.
 
- Allows mapping key combinations like `*1` to specific key sequences on the host computer.

- Easily configurable through the Arduino IDE.

### Key Mappings 

By default, the keypad has the following key mappings:
| Key Combination | Output Sequence | 
| --- | --- | 
| *1 | Ctrl+C | 
| *2 | Ctrl+V | 
| *3 | Alt+Tab | 
| *4 | Custom Sequence (editable) | 
| # | Send Enter key | 

You can modify the key mappings in the Arduino code to suit your needs.

### Setup & Installation 
 
1. **Install the Arduino IDE** : Download and install the [Arduino IDE]() .
 
2. **Install ESP32 Board Package** : Add the ESP32 board package to Arduino by following [this guide]() .
 
3. **Clone This Repository** :

```bash
git clone https://github.com/yourusername/esp32-keypad-keyboard.git
```
 
4. **Install Required Libraries** : Ensure the following libraries are installed through the Arduino Library Manager: 
  - `Keypad.h`
 
  - `USBKeyboard.h` (for ESP32-S2 USB emulation)
 
5. **Upload the Code** :
  - Open the project in the Arduino IDE.
 
  - Select the correct board (`ESP32-S2 Mini`) and port from the `Tools` menu.
 
  - Click `Upload`.

### Usage 

1. Connect the ESP32-S2 Mini to your computer using a USB cable.

2. Press key combinations on the 4x3 Matrix Keypad to trigger keyboard inputs on your computer.

3. You can modify the key mappings and sequences in the source code to suit your specific application.

## License 
This project is licensed under the GNU General Public License - see the [LICENSE]()  file for details.

---

