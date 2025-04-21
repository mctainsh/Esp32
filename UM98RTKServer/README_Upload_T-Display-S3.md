## Upload the binary to the TTGO T-Display-S3

```
IMPORTANT : This applies to ESP32-S3 only. Nt other devices like ESP32 or ESP32-S2

ALSO : T-Display and T-Display-S3 have very subtly different addresses. Pay attention.

If everything goes bad you can reset by uploading the firmware_no_touch.bin version from https://github.com/Xinyuan-LilyGO/T-Display-S3/tree/main/firmware
```

- Download the files from [TTGO T-Display-S3 Binaries](https://1drv.ms/f/s!Avrf6GYUWqyFhtUYAQn6uUMdvsv8OQ). Ensure you load the one for your ESP32 and GPS receiver

## Method 1 (Easier)

1) Open https://espressif.github.io/esp-launchpad/ which does not require anything to be downloaded to your computer.

2) Select `Connect` the choose the device in the popup

3) Select `DIY`

4) Add the files shown in the files and addresses table below

5) Select `Program`

6) Once upload completes. Jump to step `9` of Method 2.

## Method 2

1) Get the Espressif Flash Download Tools from https://www.espressif.com/en/support/download/other-tools
	- The tool is called Flash Download Tools
	- If the tool doesn't work out of the box watch https://www.youtube.com/watch?v=BVJlc4hio6A to get the correct drivers
2) Launch flash_download_tool_3.9.7.exe (Version number may be a little different)
3) Select ChipType: ESP32-S3

	<img src="https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-Display-S3/Flash-ChipSelect.png?raw=true" width="200"/>
4) Add each file to the list and address. Be sure to add a tick next to each file and the correct address

	<img src="https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-Display-S3/Flash-FileAssignment.png?raw=true" width="300"/>

	| File | Address | 
	| --- | --- | 
 	| bootloader.bin|0x0 | 
 	| partitions.bin|0x8000 | 
 	| boot_app0.bin |0xe000 |
 	| firmware.bin |0x10000|

5) Set SPI speed to 80MHz
6) Double check your work
7) Press `START`
8) If all goes well it should look like

	<img src="https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-Display-S3/Flash-CompleteResult.png?raw=true" Width="600"/>

```
NOTE : The first time you install it may take up to 90 seconds to startup after reset. Please be patient
```

9) Power cycle or hit reset. It should look like

	<img src="https://github.com/mctainsh/Esp32/blob/main/UM98RTKServer/Photos/TTGO-Display-S3/PRE_WIFI.jpg?raw=true" width="300"/>

10) If it doesn't you can reset the device with the firmware_no_touch.bin downloaded from the TTGO website
	- Don't use the ERASE button unless you want to reload the TTGO firmware