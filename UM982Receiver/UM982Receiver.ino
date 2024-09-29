/*
 
 Mac 
	/Users/{Username}/Documents/Arduino/libraries/TFT_eSPI/User_Setup_Select.h
Windows
	C:\Code\Arduino\libraries\TFT_eSPI\User_Setup_Select.h (Different on new install)
Comment out
	//#include <User_Setup.h>           // Default setup is root library folder
Uncomment
	#include <User_Setups/Setup25_TTGO_T_Display.h>    // Setup file for ESP32 and TTGO T-Display ST7789V SPI bus TFT
Add 
	#define DISABLE_ALL_LIBRARY_WARNINGS


  #########################################################################
  ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
  #########################################################################
*/
#define BUTTON_1 0
#define BUTTON_2 35
#define APP_VERSION "1.18"

#include "HandyString.h"
#include "MyDisplay.h"
#include "GpsParser.h"
#include "CredentialPrivate.h"
#include "NTRIPClient.h"

MyDisplay _display;
GpsParser _gpsParser(_display);
NTRIPClient _ntripClient(_display);

unsigned long _loopWaitTime = 0;  // Time of last second
int _loopPersSecondCount = 0;	  // Number of times the main loops runs in a second

uint8_t _button1Current = HIGH;
uint8_t _button2Current = HIGH;

bool IsButtonReleased(uint8_t button, uint8_t* pCurrent);
bool IsWifiConnected();

///////////////////////////////////////////////////////////////////////////////
// Setup
void setup(void)
{
	Serial.begin(115200);
	Serial2.begin(115200, SERIAL_8N1, 25, 26);

	pinMode(BUTTON_1, INPUT_PULLUP);
	pinMode(BUTTON_2, INPUT_PULLUP);

	_display.Setup();
	Serial.println("Startup Complete");

	// Reset the device
	//Serial2.println("freset");
}


///////////////////////////////////////////////////////////////////////////////
// Loop here
void loop()
{
	// Trigger someting every second
	int t = millis();
	_loopPersSecondCount++;
	if ((t - _loopWaitTime) > 1000)
	{
		_loopWaitTime = t;
		_display.SetLoopsPerSecond(_loopPersSecondCount);
		_loopPersSecondCount = 0;
	}

	// Check for new data GPS serial data
	_gpsParser.ReadDataFromSerial(Serial2);

	// WIFI related functions
	if (IsWifiConnected())
	{
		// Check for new RTK corrections if we have a location already
		if (_display.HasLocation())
			_ntripClient.Loop();
	}

	// Check for push buttons
	if (IsButtonReleased(BUTTON_1, &_button1Current))
		_display.NextPage();
	if (IsButtonReleased(BUTTON_2, &_button2Current))
		_display.ToggleDeltaMode();

	// Update animations
	_display.Animate();
}

///////////////////////////////////////////////////////////////////////////////
// Check if the button is released. This takes < 1ms
// Note : Button is HIGH when released
bool IsButtonReleased(uint8_t button, uint8_t* pCurrent)
{
	if (*pCurrent != digitalRead(button))
	{
		*pCurrent = digitalRead(button);
		return *pCurrent == HIGH;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// Check Wifi and reconnect
bool IsWifiConnected()
{
	// Is the WIFI connected?
	if (WiFi.status() == WL_CONNECTED)
		return true;

	// start the connection process
	Serial.println("E310 - No WIFI");
	WiFi.mode(WIFI_STA);
	wl_status_t beginState = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	Serial.printf("WiFi Connecting %d \r\n", beginState);

	// Wait 20 seconds to connect
	int countDown = 40;
	while (WiFi.status() != WL_CONNECTED)
	{
		_display.SetWebStatus(StringPrintf("WiFi %d %d", beginState, countDown));
		delay(500);
		if (countDown-- < 1)
		{
			Serial.println("E311 - WIFI Connect timmed out");
			_display.SetWebStatus("NO WIFI!!!");
			return false;
		}
	}
	_display.SetWebStatus(WiFi.localIP().toString().c_str());
	return true;
}
