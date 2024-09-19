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
 
3. **USB-C Cable**  - Connects the ESP32 to the computer for both power and communication.
 
4. **Jumper Wires**  - For connecting the keypad to the ESP32.

### Wiring Diagram 
| Keypad Pin | ESP32-S2 Pin | 
| --- | --- | 
| R1 | GPIO 10 | 
| R2 | GPIO 1 | 
| R3 | GPIO 2 | 
| R4 | GPIO 6 | 
| C1 | GPIO 8 | 
| C2 | GPIO 13 | 
| C3 | GPIO 4 | 

- **R1 - R4** : Rows of the matrix keypad
 
- **C1 - C3** : Columns of the matrix keypad

It is a happy conicidence that the ESP32 can be soldered directly to the back of the keypad and the pinouts will align

![Pinouts](https://github.com/mctainsh/Esp32/blob/main/MarcoKeyboard/Photos/KeypadBackDetail.jpg?raw=true)

![Front](https://github.com/mctainsh/Esp32/blob/main/MarcoKeyboard/Photos/S2%20from%20side.jpg?raw=true)


## Software 

### Features 

- Emulates a USB keyboard using the ESP32-S2 Mini.
 
- Allows mapping key combinations like `*1` to specific key sequences on the host computer.

- Easily configurable through the Arduino IDE.

### Key Mappings 

By default, the keypad has the following key mappings:
| Key Combination | Output Sequence | 
| --- | --- | 
| *1 | McLovin@phantom.com | 
| *2 | McLovin\nGoogle Drive\n212 Tahakapoa Valley Rd\nOtago, New Zealand | 
| **4 | QuickLogin@1234567898\n | 
| **5 | mclovin\tPassword!\n | 

You can modify the key mappings in the Actions.h file to suit your needs. Be careful placing passwords in this file if you are going to check it into source control.

Each action is made up of the following
| Member | Description | 
| --- | --- | 
| Command | This is the sequence of keys to hit to activate the command. Note : There is a 2.5 second delay to reset the command sequence. |
| Backspace | Number of backspaces to send before the commands. Handy for clearing out the macro keys or deleting any failed password attempts |
| Macro | Keys sent to the keyboard when the marco activates. handy additions are \t for tab and \n for enter | 

### Setup & Installation 
 
1. **Install the Arduino IDE** : Download and install the [Arduino IDE](https://www.arduino.cc/en/software) .
 
2. **Install ESP32 Board Package** : Add the ESP32 board package to Arduino by following [this guide](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html) .
 
3. **Clone This Repository** :

```bash
git clone https://github.com/mctainsh/Esp32.git
```

or just copy the files from
```
https://github.com/mctainsh/Esp32/tree/main/MarcoKeyboard/MarcoKeyboard
```
 
4. **Install Required Libraries** : Ensure the following libraries are installed through the Arduino Library Manager (These may already be there):
  - `USB.h`
 
  - `USBHIDKeyboard.h` (for ESP32-S2 USB emulation)
 
5. **Upload the Code** :
  - Open the project in the Arduino IDE.
 
  - Select the correct board (`LOLIN ESP32-S2 Mini`) and port from the `Select Board` drop down.
 
  - Click `Upload`.
  
  - Note : The first time you try to program the ESP32, it may have been preloaded with some freeky python or other stuff. If it does not upload on the first try. Hold the EN button and tap the RST button whilr plugged into the Android IDE.

### Usage 

1. Connect the ESP32-S2 Mini to your computer using a USB cable.

2. Press key combinations on the 4x3 Matrix Keypad to trigger keyboard inputs on your computer.

3. You can modify the key mappings and sequences in the source code to suit your specific application.

## License 
This project is licensed under the GNU General Public License - see the [LICENSE]()  file for details.


![Back](https://github.com/mctainsh/Esp32/blob/main/MarcoKeyboard/Photos/KeypadBack.jpg?raw=true)


![Front](https://github.com/mctainsh/Esp32/blob/main/MarcoKeyboard/Photos/KeypadFront.jpg?raw=true)



---

