#include <Arduino.h>
///////////////////////////////////////////////////////////////////////////////
//	  #########################################################################
//	  ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
//	  #########################################################################
//
//	To use the TTGO T-Display or T-Display-S3 with the TFT_eSPI library, you need to make the following changes to
//	the .pio\libdeps\lilygo-t-display\TFT_eSPI\User_Setup_Select.h file in the library.
//
//	1. Comment out the default setup file
//		//#include <User_Setup.h>           // Default setup is root library folder
//
//	2. Uncomment the TTGO T-Display setup file
//		#include <User_Setups/Setup25_TTGO_T_Display.h>    		// Setup file for ESP32 and TTGO T-Display ST7789V SPI bus TFT
//	  	OR
//		#include <User_Setups/Setup206_LilyGo_T_Display_S3.h>     // For the LilyGo T-Display S3 based ESP32S3 with ST7789 170 x 320 TFT
//
//	3. Add the following line to the start of the file
//		#define DISABLE_ALL_LIBRARY_WARNINGS
///////////////////////////////////////////////////////////////////////////////

#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>

#include <WiFi.h>
#include <iostream>
#include <sstream>
#include <string>

#include "Global.h"
#include "HandyLog.h"
#include "HandyString.h"
#include "MyDisplay.h"
#include "GpsParser.h"
#include "CredentialPrivate.h"
#include "NTRIPServer.h"
#include "MyFiles.h"

MyFiles _myFiles;
MyDisplay _display;
GpsParser _gpsParser(_display);
NTRIPServer _ntripServer0(_display, 0, CASTER_0_ADDRESS, CASTER_0_PORT, CASTER_0_CREDENTIAL, CASTER_0_PASSWORD);
NTRIPServer _ntripServer1(_display, 1, CASTER_1_ADDRESS, CASTER_1_PORT, CASTER_1_CREDENTIAL, CASTER_1_PASSWORD);

unsigned long _loopWaitTime = 0; // Time of last second
int _loopPersSecondCount = 0;	 // Number of times the main loops runs in a second

uint8_t _button1Current = HIGH; // Top button on left
uint8_t _button2Current = HIGH; // Bottom button when

// WiFi monitoring states
#define WIFI_STARTUP_TIMEOUT 20000
unsigned long _wifiFullResetTime = -WIFI_STARTUP_TIMEOUT;
wl_status_t _lastWifiStatus = wl_status_t::WL_NO_SHIELD;

bool IsButtonReleased(uint8_t button, uint8_t *pCurrent);
bool IsWifiConnected();

///////////////////////////////////////////////////////////////////////////////
// Setup
void setup(void)
{
	Serial.begin(115200);
#if USER_SETUP_ID == 25
	Serial2.begin(115200, SERIAL_8N1, 25, 26);
#else
	Serial2.begin(115200, SERIAL_8N1, 12, 13);
	// Turn on display power for the TTGO T-Display-S3 (Needed for battery operation or if powered from 5V pin)
	pinMode(15, OUTPUT);
	digitalWrite(15, HIGH);
#endif

	Logf("Starting %s\r\n", APP_VERSION);

	pinMode(BUTTON_1, INPUT_PULLUP);
	pinMode(BUTTON_2, INPUT_PULLUP);

	// Verify file IO
	if (_myFiles.Setup())
	{
		Logln("Test file IO");
		//_myFiles.WriteFile("/hello.txt", "Hello ");
		//_myFiles.AppendFile("/hello.txt", "World!\r\n");

		// std::string response;
		//_myFiles.ReadFile("/hello.txt", response);
		// Logln(response.c_str());
	}
	else
	{
		Logln("E100 - File IO failed");
	}

	_display.Setup();
	Logln("Startup Complete");
}

///////////////////////////////////////////////////////////////////////////////
// Loop here
void loop()
{
	// Trigger something every second
	int t = millis();
	_loopPersSecondCount++;
	if ((t - _loopWaitTime) > 1000)
	{
		_loopWaitTime = t;
		_display.SetLoopsPerSecond(_loopPersSecondCount, t);
		_loopPersSecondCount = 0;		
	}	

	// Check for push buttons
	if (IsButtonReleased(BUTTON_1, &_button1Current))
	{
		Logln("Button 1");
		_display.NextPage();
	}
	if (IsButtonReleased(BUTTON_2, &_button2Current))
	{
		Logln("Button 2");
		_display.ActionButton();
	}

	// Check for new data GPS serial data
	if (IsWifiConnected())
		_display.SetGpsConnected(_gpsParser.ReadDataFromSerial(Serial2, _ntripServer0, _ntripServer1) );
	else
		_display.SetGpsConnected(false);

	// WIFI related functions
	//	if (IsWifiConnected())
	//	{
	// Check for new RTK corrections if we have a location already
	//		if (_display.HasLocation())
	//			_ntripServer.Loop();
	//	}

	// Update animations
	_display.Animate();
}

///////////////////////////////////////////////////////////////////////////////
// Check if the button is released. This takes < 1ms
// Note : Button is HIGH when released
bool IsButtonReleased(uint8_t button, uint8_t *pCurrent)
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
	wl_status_t status = WiFi.status();
	if (_lastWifiStatus != status)
	{
		_lastWifiStatus = status;
		Logf("Wifi Status %d %s\r\n", status, WifiStatus(status));
	//	_display.SetWebStatus(status);
		_display.RefreshWiFiState();
//		if (status == WL_CONNECTED)
//			_display.SetRtkStatus(0, "GPS Pending");
	}

	if (status == WL_CONNECTED)
		return true;

	// Start the connection process
	// Logln("E310 - No WIFI");
	unsigned long t = millis();
	unsigned long tDelta = t - _wifiFullResetTime;
	if (tDelta < WIFI_STARTUP_TIMEOUT)
	{

		// if( status != WL_DISCONNECTED)
		//	Logln(stateTitle.c_str());
		//_display.SetWebStatus(stateTitle.c_str());

		// Create a string of dots to show progress. 1 dot per second
	//	std::string dotsStr(tDelta / 1000, '.');
	//	_display.SetRtkStatus(0, dotsStr.c_str());

		return false;
	}

	// Reset the WIFI
	_wifiFullResetTime = t;
	WiFi.mode(WIFI_STA);
	wl_status_t beginState = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	//_display.SetWebStatus(WifiStatus(beginState));
	Logf("WiFi Connecting %d %s\r\n", beginState, WifiStatus(beginState));
	_display.RefreshWiFiState();

	return false;
}

//  Check the Correct TFT Display Type is Selected in the User_Setup.h file
#if USER_SETUP_ID != 206 && USER_SETUP_ID != 25
#error "Error! Please make sure the required LILYGO PCB in .pio\libdeps\lilygo-t-display\TFT_eSPI\User_Setup_Select.h See top of main.cpp for details"
#error "Error! Please make sure the required LILYGO PCB in .pio\libdeps\lilygo-t-display\TFT_eSPI\User_Setup_Select.h See top of main.cpp for details"
#error "Error! Please make sure the required LILYGO PCB in .pio\libdeps\lilygo-t-display\TFT_eSPI\User_Setup_Select.h See top of main.cpp for details"
#endif