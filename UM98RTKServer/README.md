# UM982 RTK Server with TTGO T-DisplayESP32-S3

WARNING :  Do not run without real credentials or your IP may be blocked!!

![Wiring](https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/Wiring.png?raw=true)

This project connects a UM982 RTK GNSS receiver to a TTGO T-Display-S3 allowing you to share RTK correction data with various networks.

NOTE : ALthough the code is able to send data to two RTK casters, if one of the casters fails to receive the message the other will be delayed. I'm working on a better option where I daisy chain ESP-S2-Minis to handel more casters.

I haven't included the STL for box as it is a mess. When I do a nice one, I will include it.

## TODO

1. Write instructions to install without compiling with PlatformIO (Using ESP32 Upload tool)

2. Make http sends in non-blocking to prevent one NTRIP server upsetting the others

3. Rework the TTGO T-Display-S2 code to make the display nicer (Currently optimized for larger S3)

4. Put each NTRIP server details on its own page

5. Make better looking STL

6. Build one using ESP32-S3 Mini board. Won't have display but will be very compact

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

This project enables an TTGO T-Display to act as an RTK server sending RTK corrections to two casters. These could be Onocoy, Rtk2Go or RtkDirect.

### Terms

| Name | Description |
| --- | --- |
| RTK Client | A device or software that receives RTK correction data from a server to improve positioning accuracy. |
| RTK Server | A server that processes and distributes RTK correction data to clients. (This project builds a RTK Server) |
| RTK Caster | A service that broadcasts RTK correction data over the internet using the NTRIP protocol. |

### Use Cases 

- Creating accurate GPS data where there is none for very cheap

## Hardware 

### Components 
 
1. **UM982 with antenna** - Witte Intelligent WTRTK-982 high-precision positioning and orientation module. I got it from AliExpress for about A$220.00 [Not affiliate link. Find your own seller](https://www.aliexpress.com/item/1005007287184287.html)
 
2.1 **TTGO T-Display-S3** - (Preferred) ESP32 S3 with Display AliExpress for A$32.00. [Not an affiliate link shop with care](https://www.aliexpress.com/item/1005004496543314.html)

2.2 **TTGO T-Display-S2** - ESP32 S3 with Display AliExpress for A$13.19. [Not an affiliate link shop with care](https://www.aliexpress.com/item/1005005970553639.html)
 
3. **Wires Protoboard** - Connects the ESP32 to receiver as described below.

4. **Fan** - Used a 5V fan cos the UM982 get hot in the sun.
 

### Wiring Diagram

#### TTGO T-Display-S3
| TTGO T-Display-S3 Pin | Use | UM982 pin | Use |
| --- | --- | --- | --- |
| 5V | 5V| 2 | 5V |
| G | GND | 5 | GND |
| NC | |  |  |
| NC | |  |  |
| 13 | TX | 3 | RX |
| 12 | RX | 4 | TX |

#### TTGO T-Display-S2
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

- Programs the UM982 to send generate RTK correction data

- Sends correction data to both RTK Casters

### Upload binary (Broken)

Do not use the following command, it is only here as example
"C:\Users\YOUR_NAME\.platformio\penv\Scripts\python.exe" "C:\Users\YOUR_NAME\.platformio\packages\tool-esptoolpy\esptool.py" --chip esp32s3 --port "COM16" --baud 460800 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 16MB 0x0000 bootloader.bin 0x8000 partitions.bin 0xe000 C:\Users\YOUR_NAME\.platformio\packages\framework-arduinoespressif32\tools\partitions\boot_app0.bin 0x10000 .pio\build\lilygo-t-display-s3\firmware.bin

### Config parameters 

Config parameters are set in the "Configure Wifi" web page
| Parameter | Usage | 
| --- | --- | 
| SSID | Your WiFi network name. Note: Not all speed are supported by ESP32 |
| Password | Your Wifi password |
| CASTER 1 ADDRESS | Usually "ntrip.rtkdirect.com" |
| CASTER 1 PORT | Port usually 2101 |
| CASTER 1 CREDENTIAL | This is the reference station Credential. NOT the Mount point name |
| CASTER 1 PASSWORD | Sign up to get this from Onocoy |
| CASTER 2 ADDRESS | Usually "servers.onocoy.com" |
| CASTER 2 PORT | Port usually 2101 |
| CASTER 2 CREDENTIAL | This is the reference station Credential. NOT the Mount point name |
| CASTER 2 PASSWORD | Sign up to get this from Onocoy |
| CASTER_3 ADDRESS | Usually "rtk2go.com" |
| CASTER 3 PORT | Port usually 2101 |
| CASTER 3 CREDENTIAL | Mount point name |
| CASTER 3 PASSWORD | Create this with Rtk2Go signup |


### Setup & Installation 

1. **Install VS Code** : Follow [Instructions](https://code.visualstudio.com/docs/setup/setup-overview)

2. **Install the PlatformIO IDE** : Download and install the [PlatformIO](https://platformio.org/install).
 
3. **Clone This Repository**

```bash
git clone https://github.com/mctainsh/Esp32.git
```

or just copy the files from
```
https://github.com/mctainsh/Esp32/tree/main/UM98RTKServer/UM98RTKServer
```
4. **Enable the TTGO T-Display header** : To use the TTGO T-Display-S3 with the TFT_eSPI library, you need to make the following changes to the User_Setup.h file in the library.

```
	.pio\libdeps\lilygo-t-display\TFT_eSPI\User_Setup_Select.h
	4.1. Comment out the default setup file
		//#include <User_Setup.h>           // Default setup is root library folder
	4.2. Uncomment the TTGO T-Display-S3 setup file
		#include <User_Setups/Setup206_LilyGo_T_Display_S3.h>     // For the LilyGo T-Display S3 based ESP32S3 with ST7789 170 x 320 TFT
	4.3. Add the following line to the start of the file
		#define DISABLE_ALL_LIBRARY_WARNINGS
```

### Configuration 

1. Create the accounts with [Oncony register](https://console.onocoy.com/auth/register/personal), [RtkDirect](https://cloud.rtkdirect.com/) or [RTK2GO](http://rtk2go.com/sample-page/new-reservation/)

2. Don't wire up anything to start with (We can let the smoke out of it later)

3. Upload the program to your ESP32. 

4. Power it up and check display for WIFI connection status.

5. Following instruction at [WifiManager](https://github.com/tzapu/WiFiManager) to connect your ESP32 to your WIFI.

6. Configure the RTK Servers you want to use in the "Configure Wifi" Screen.

7. Wire up the ESP32 to UM98x. Power it fom UM98x (Sometime the ESP32 doesn't output enough beans).

8. Review the status and logs through the web interface (http://x.x.x.x/i)

### Important

The T-Display-S3 will turn off it's display after about 30 seconds. This is OK, just press either button to turn it on again.

### Display

The display has several screens you can toggle through them by pressing one of the buttons.

The top line of the display shows the following

| Type | Usage | 
| --- | --- | 
| / | Rotating animation to show main loop is running |
| Title | Title of the page currently displayed |
| X | Connection state of RTK Server 3 |
| X | Connection state of RTK Server 2 |
| X | Connection state of RTK Server 1 |
| X | Connection state of WIFI |
| X | Connection State to UM98x |

![Home](https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-S3-Display/S3-Screen-Home.jpg?raw=true)

![GPS](https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-S3-Display/S3-Screen-GPS.jpg?raw=true)

![RTK](https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-S3-Display/S3-Screen-RTK.jpg?raw=true)

![GPS Log](https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-S3-Display/S3-Screen-Log-GPS.jpg?raw=true)

![Server 1 Log](https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-S3-Display/S3-Screen-Log-C1.jpg?raw=true)

![Server 2 Log](https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-S3-Display/S3-Screen-Log-C2.jpg?raw=true)

![Server 3 Log](https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-S3-Display/S3-Screen-Log-C3.jpg?raw=true)


## License 
This project is licensed under the GNU General Public License - see the [LICENSE](https://github.com/mctainsh/Esp32/blob/main/LICENSE)  file for details.

## More photos
![T-Display-S3](https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/T-DISPLAY-S3.jpg?raw=true)

![UM982](https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/UM982.png?raw=true)

![Dimensions](https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/UM982-PCB.png?raw=true)

---

