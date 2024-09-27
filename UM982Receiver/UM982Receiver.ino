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

#include "HandyString.h"
#include "MyDisplay.h"
#include "GpsParser.h"
#include "Credentials.h"
#include "NTRIPClient.h"


#define BUTTON_1 0
#define BUTTON_2 35

MyDisplay _display;
GpsParser _gpsParser(_display);
NTRIPClient _ntripClient(_display);

unsigned long targetTime = 0;
int _count;

uint8_t _button1Current = HIGH;
uint8_t _button2Current = HIGH;

bool IsButtonReleased(uint8_t button, uint8_t* pCurrent);


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
	Serial2.println("freset");
}


///////////////////////////////////////////////////////////////////////////////
// Loop here
void loop()
{
	// Is the WIFI connected?
	if (WiFi.status() != WL_CONNECTED)
	{
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
			if( countDown-- < 1 )
			{
				Serial.println("E311 - WIFI Connect timmed out");
				_display.SetWebStatus("NO WIFI!!!");
				return;
			}
		}
		_display.SetWebStatus(WiFi.localIP().toString().c_str());
	}

	// Check for new data GPS serial data
	_gpsParser.ReadDataFromSerial(Serial2);

	// CHeck for new RTK corrections
	_ntripClient.Loop();

	// Check for push buttons
	if (IsButtonReleased(BUTTON_1, &_button1Current))
		_display.ToggleDeltaMode();
	//if (IsButtonReleased(BUTTON_2, &_button2Current))
	//	n--;

	_count++;


	if (targetTime > millis())
		return;

	targetTime = millis() + 1000;

	_display.SetCount(_count);
	//_display.OnDraw();

	_count = 0;
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
