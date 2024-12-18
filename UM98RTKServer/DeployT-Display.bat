cd C:\Code\Arduino\Public\Esp32\UM98RTKServer\
md C:\Code\Arduino\Public\Esp32\UM98RTKServer\Deploy
md C:\Code\Arduino\Public\Esp32\UM98RTKServer\Deploy\T-Display
cd C:\Code\Arduino\Public\Esp32\UM98RTKServer\Deploy\T-Display
copy C:\Code\Arduino\Public\Esp32\UM98RTKServer\.pio\build\lilygo-t-display\bootloader.bin
copy C:\Code\Arduino\Public\Esp32\UM98RTKServer\.pio\build\lilygo-t-display\partitions.bin 
copy C:\Code\Arduino\Public\Esp32\UM98RTKServer\.pio\build\lilygo-t-display\firmware.bin
copy C:\Users\john\.platformio\packages\framework-arduinoespressif32\tools\partitions\boot_app0.bin
pause
