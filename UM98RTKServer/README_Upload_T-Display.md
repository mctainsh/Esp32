## Upload the binary to the TTGO T-Display

```
IMPORTANT : This applies to ESP32 only. Not other devices like ESP32-S2 or ESP32-S3
```

- Download the files from [TTGO T-Display Binaries](https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Deploy/T-Display)
- Get the Espressif Flash Download Tools from https://www.espressif.com/en/support/download/other-tools
	- The tool is called Flash Download Tools
	- If the tool doesn't work out of the box watch https://www.youtube.com/watch?v=BVJlc4hio6A to get the correct drivers
- Launch flash_download_tool_3.9.7.exe (Version number may be a little different)
- Select ChipType: ESP32
<img src="https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-Display/Flash-Flash-ChipSelect.png?raw=true"/>
- Add each file to the list
- Be sure to add a tick next to each file and the correct address
<img src="https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-Display/Flash-FileAssignment.png?raw=true"/>
- Double check your work
- Press `START`
- If all goes well it should look like
<img src="https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-Display/Flash-CompleteResult.png?raw=true"/>
- Power cycle or hit reset. It should look like
<img src="https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-Display/PRE_WiFi.jpg?raw=true"/>
- If it doesn't you can reset the device with the firmware.bin downloaded from the TTGO website
	- Don't use the ERASE button unless you want to reload the TTGO firmware