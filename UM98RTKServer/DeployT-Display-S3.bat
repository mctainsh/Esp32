cd C:\Code\Arduino\Public\Esp32\UM98RTKServer\
md C:\Code\Arduino\Public\Esp32\UM98RTKServer\Deploy
md C:\Code\Arduino\Public\Esp32\UM98RTKServer\Deploy\T-Display-S3
cd C:\Code\Arduino\Public\Esp32\UM98RTKServer\Deploy\T-Display-S3
copy C:\Code\Arduino\Public\Esp32\UM98RTKServer\.pio\build\lilygo-t-display-s3\bootloader.bin
copy C:\Code\Arduino\Public\Esp32\UM98RTKServer\.pio\build\lilygo-t-display-s3\partitions.bin 
copy C:\Code\Arduino\Public\Esp32\UM98RTKServer\.pio\build\lilygo-t-display-s3\firmware.bin
copy C:\Users\john\.platformio\packages\framework-arduinoespressif32\tools\partitions\boot_app0.bin
pause

