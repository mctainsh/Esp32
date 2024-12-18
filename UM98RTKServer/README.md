# UM982 RTK Server with TTGO T-Display-S3 (Multi miner)

This project connects a UM980 or UM982 RTK GNSS receiver to a TTGO T-Display/-S3 allowing you to share RTK correction data with up to three networks at one time (Some give mining rewards). The ESP32 will automatically program the UM980/2 so there is no need to mess around with terminals or or the UPrecise software.

All up you it will cost about US$200 to make the station with GNSS receiver, antenna and ESP32 with display. 

If you want to send to more than three casters you can connect a second ESP32 in parallel to the TX port of the UM98x and power both ESP32's at the some time. No need for a second UM98x or expensive splitters.

The display also allow you to see at an instant if the system is connected and sending to all the casters

<div style="display: flex; gap:10px;">
	<img src="https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-S3-Display/S3-Screen-Home.jpg?raw=true" width="180" />
	<img src="https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-S3-Display/S3-Screen-GPS.jpg?raw=true" width="180" />
	<img src="https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-S3-Display/S3-Screen-RTK.jpg?raw=true" width="180" />
</div>

NOTE : Although the code is able to send data to three RTK casters, if one of the casters fails to receive the message (blocks) the other will be delayed. I'm working on a better option to make non blocking socket calls or daisy chain ESP-S2-Mini's to handle more casters.

<img src="https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-S3-Display/T-Display-S3-UM982_Boxed.jpg?raw=true" width="400" />


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

This project enables an TTGO T-Display to act as an RTK server sending RTK corrections to up to three casters. Examples of these are be Onocoy, Rtk2Go or RtkDirect.

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

2.2 **TTGO T-Display** - ESP32 LX6 with Display AliExpress for A$13.19. [Not an affiliate link shop with care](https://www.aliexpress.com/item/1005005970553639.html)
 
3. **Wires Protoboard** - Connects the ESP32 to receiver as described below.

4. **Fan** - Used a 5V fan cos the UM982 get hot in the sun.
 

## Wiring Diagram

### TTGO T-Display-S3
| TTGO T-Display-S3 Pin | Use | UM982 pin | Use |
| --- | --- | --- | --- |
| 5V | 5V| 2 | 5V |
| G | GND | 5 | GND |
| NC | |  |  |
| NC | |  |  |
| 13 | TX | 3 | RX |
| 12 | RX | 4 | TX |


<div style="display:flex;">
	<img src="https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-S3-Display/T-DIsplay-S3_Schematic.jpg?raw=true" Height="320" />
	<img src="https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/Wiring.png?raw=true" Height="320" />
</div>

### TTGO T-Display

| TTGO T-Display Pin | Use | UM982 pin | Use |
| --- | --- | --- | --- |
| 5V | 5V| 2 | 5V |
| G | GND | 5 | GND |
| NC | |  |  |
| 26 | TX | 3 | RX |
| 25 | RX | 4 | TX |

<img src="https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-Display/T-DIsplay_Schematic.jpg?raw=true" width="400" />


## Software 

### Features 

- Connected to UM982.
 
- Connects to Wifi.

- Programs the UM982 to send generate RTK correction data

- Sends correction data to both RTK Casters

### ESP32 device setup

Depending on the device you will need to upload the binary

[Upload Binary for TTGO T-Display](https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/README_Upload_T-Display.md)


### Connect WIFI

- Connect ESP32 to your HOME WiFi
	- Power up the TTGO-Display (GPS does not need to be connected)
	- Using your phone or computer
	- Connect to the WIFI access point Rtk_XXXXXXXX (Mac address shown on ESP32 screen)
	- Password will be `JohnTLegend`
	- Browse to http://192.168.4.1
	- Select "Configure WiFi"
	- Select your HOME WiFi network and enter credential
- Connect your phone or computer to you HOME WiFi 
	- You should be able to browse to the WiFi address shown on the ESP32 screen (NOT 192.168.4.1)




### Config parameters 

Config parameters are set in the "Configure Wifi" web page

Note : You don't need to sign up to all three. Leave the CASTER ADDRESS blank to only use one or two casters. 

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

WARNING :  Do not run without real credentials or your IP may be blocked!!

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


### General
<img src="https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-S3-Display/S3-Screen-Home.jpg?raw=true" width="300"/>

| Title | Meaning | 
| --- | --- | 
| Wi-Fi | Wifi IP address. | 
| Version | Software version | 
| Up time | How log it has been running. Max 76 days before the counter rolls over  | 
| Speed | Now many times to process is being checked per second | 

### GPS State
<img src="https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-S3-Display/S3-Screen-GPS.jpg?raw=true" width="300"/>

| Title | Meaning | 
| --- | --- | 
| Type | Type of GPS device. Queried at startup | 
| Resets |  | 
| Packets | How many packets have been received | 
| Serial # | GPS module serial number | 
| Firmware | GPS module firmware verison | 

### RTK Server

<img src="https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-S3-Display/S3-Screen-RTK.jpg?raw=true" width="300"/>

Only shows the state of the first two casters

| Title | Meaning | 
| --- | --- | 
| State | Connection state | 
| Reconnect | Number of time the connection was lost | 
| Sends | Number of packets sent | 
| μs | Microseconds per byte sent | 

### GPS Log

<img src="https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-S3-Display/S3-Screen-Log-GPS.jpg?raw=true" width="300"/>


### First RTK Caster Log

<img src="https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-S3-Display/S3-Screen-Log-C1.jpg?raw=true" width="300"/>

### Second RTK Caster Log

<img src="https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-S3-Display/S3-Screen-Log-C2.jpg?raw=true" width="300"/>

### Third RTK Caster Log

<img src="https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-S3-Display/S3-Screen-Log-C3.jpg?raw=true" width="300"/>

## TODO

1. Write instructions to install without compiling with PlatformIO (Using ESP32 Upload tool)

2. Make http sends in non-blocking to prevent one NTRIP server upsetting the others

3. Rework the TTGO T-Display code to make the display nicer (Currently optimized for larger S3)

4. Put each NTRIP server details on its own page

5. Make better looking STL

6. Build one using ESP32-S3 Mini board. Won't have display but will be very compact

## License 
This project is licensed under the GNU General Public License - see the [LICENSE](https://github.com/mctainsh/Esp32/blob/main/LICENSE)  file for details.

## More photos
![T-Display-S3](https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/T-DISPLAY-S3.jpg?raw=true)

![UM982](https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/UM982.png?raw=true)

![Dimensions](https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/UM982-PCB.png?raw=true)

---

