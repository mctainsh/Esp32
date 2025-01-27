## Upload the binary to the TTGO T-Display-S3

```
IMPORTANT : This applies to ESP32-S3 only. Not other devices like ESP32 or ESP32-S2

ALSO : T-Display and T-Display-S3 have very subtly different addresses. Pay attention.

If everything goes bad you can reset by uploading the firmware_no_touch.bin version from https://github.com/Xinyuan-LilyGO/T-Display-S3/tree/main/firmware
```

NOTE: You can also use https://espressif.github.io/esp-launchpad/ which does not require anything to be downloaded to your computer.


- Download the files from [TTGO T-Display-S3 Binaries](https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Deploy/T-Display-S3)
- Get the Espressif Flash Download Tools from https://www.espressif.com/en/support/download/other-tools
	- The tool is called Flash Download Tools
	- If the tool doesn't work out of the box watch https://www.youtube.com/watch?v=BVJlc4hio6A to get the correct drivers
- Launch flash_download_tool_3.9.7.exe (Version number may be a little different)
- Select ChipType: ESP32-S3

	<img src="https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-Display-S3/Flash-ChipSelect.png?raw=true" width="200"/>
- Add each file to the list and address. Be sure to add a tick next to each file and the correct address

	<img src="https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-Display-S3/Flash-FileAssignment.png?raw=true" width="300"/>

	| File | Address | 
	| --- | --- | 
 	| bootloader.bin|0x0 | 
 	| partitions.bin|0x8000 | 
 	| boot_app0.bin |0xe000 |
 	| firmware.bin |0x10000|

- Set SPI speed to 80MHz
- Double check your work
- Press `START`
- If all goes well it should look like

	<img src="https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-Display-S3/Flash-CompleteResult.png?raw=true" Width="600"/>

```
NOTE : The first time you install it may take up to 90 seconds to startup after reset. Please be patient
```

- Power cycle or hit reset. It should look like

	<img src="https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-Display-S3/PRE_WIFI.jpg?raw=true" width="300"/>
- If it doesn't you can reset the device with the firmware_no_touch.bin downloaded from the TTGO website
	- Don't use the ERASE button unless you want to reload the TTGO firmware