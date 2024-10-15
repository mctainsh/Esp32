# UM982 RTK Tracker with TTGO T-DisplayESP32-S2 Mini Keypad to Keyboard Emulator 

![Case](https://github.com/mctainsh/Esp32/blob/main/UM982Tracker/Photos/Case.jpg?raw=true)

This project connects a UM982 RTK GNSS receiver to a TTGO T-Display ESP32 allow 1cm accuracy positioning.

I haven't included the STL for box as it is a mess. When I do a nice one, I will include it.

## TODO

1. Make config parameters configurable through RS232 connection

2. Make wifi connection configurable through direct WIFI connection.

3. Add speed and direciton.

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

- Surveying or accurate measurement gathering

## Hardware 

### Components 
 
1. **UM982 with antenna** - Witte Intelligent WTRTK-982 high-precision positioning and orientation module. I got it from AliExpress for about $220.00 [Not affiliate link. Find your own seller](https://www.aliexpress.com/item/1005007287184287.html)
 
2. **TTGO T-Display** - ESP32 S@ with Display AliExpress for $12.00. [Not an affiliate link shop with care](https://www.aliexpress.com/item/1005006373354300.html)
 
3. **Wires Protoboard**  - Connects the ESP32 to receiver as described below.

4. **Fan** - Used a 5V fan cos the UM982 get hot in the sun.
 

### Wiring Diagram (4x3)
| TTGO T-Display Pin | Use | UM982 pin | Use |
| --- | --- | --- | --- |
| 5V | 5V| 2 | 5V |
| G | GND | 5 | GND |
| NC | |  |  |
| 26 | TX | 3 | RX |
| 25 | RX | 4 | TX |



## Software 

### Features 

- Connected to UM982.
 
- Connects to Wifi.

- Connects to NTRIP service for RTK corrections.

- Unwraps RTK corrections and send them to UM982.

- Sends location data in realtime to web URL.

### Config parameters 

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

1. **Install VS Code** : Follow [Instructions](https://code.visualstudio.com/docs/setup/setup-overview)

2. **Install the PlatformIO IDE** : Download and install the [PlatformIO](https://platformio.org/install).
 
3. **Clone This Repository**

```bash
git clone https://github.com/mctainsh/Esp32.git
```

or just copy the files from
```
https://github.com/mctainsh/Esp32/tree/main/UM982Tracker/UM982Tracker
```

4. **Enable the TTGO T-Display header** : To use the TTGO T-Display with the TFT_eSPI library, you need to make the following changes to the User_Setup.h file in the library.
	.pio\libdeps\lilygo-t-display\TFT_eSPI\User_Setup_Select.h
	4.1. Comment out the default setup file
		//#include <User_Setup.h>           // Default setup is root library folder
	4.2. Uncomment the TTGO T-Display setup file
		#include <User_Setups/Setup25_TTGO_T_Display.h>    // Setup file for ESP32 and TTGO T-Display ST7789V SPI bus TFT
	4.3. Add the following line to the start of the file
		#define DISABLE_ALL_LIBRARY_WARNINGS



### Usage 

1. Connect the ESP32-S2 Mini to your computer using a USB cable.

2. Press key combinations on the 4x3 Matrix Keypad to trigger keyboard inputs on your computer.

3. You can modify the key mappings and sequences in the source code to suit your specific application.

4. If you type an incorrect sequence, wait 2.5 seconds and the command builder will reset. The keys will remain on the screen as this may have been your intention?

## License 
This project is licensed under the GNU General Public License - see the [LICENSE](https://github.com/mctainsh/Esp32/blob/main/LICENSE)  file for details.

## More photos
![Case](https://github.com/mctainsh/Esp32/blob/main/UM982Tracker/Photos/Case.jpg?raw=true)


![UM982](https://github.com/mctainsh/Esp32/blob/main/UM982Tracker/Photos/UM982.png?raw=true)



---

