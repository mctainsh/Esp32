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

#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include "Global.h"
#include "HandyLog.h"
#include "HandyString.h"
#include "MyDisplay.h"
#include "GpsParser.h"
#include "CredentialPrivate.h"
#include "NTRIPServer.h"
#include "MyFiles.h"
#include <WebPortal.h>

WiFiManager _wifiManager;

unsigned long _loopWaitTime = 0;	// Time of last second
int _loopPersSecondCount = 0;		// Number of times the main loops runs in a second
unsigned long _lastButtonPress = 0; // Time of last button press to turn off display on T-Display-S3

WebPortal _webPortal;

uint8_t _button1Current = HIGH; // Top button on left
uint8_t _button2Current = HIGH; // Bottom button when

MyFiles _myFiles;
MyDisplay _display;
GpsParser _gpsParser(_display);
NTRIPServer _ntripServer0(_display, 0);
NTRIPServer _ntripServer1(_display, 1);
NTRIPServer _ntripServer2(_display, 2);

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
#if T_DISPLAY_S3 == true
	Serial2.begin(115200, SERIAL_8N1, 12, 13);
	// Turn on display power for the TTGO T-Display-S3 (Needed for battery operation or if powered from 5V pin)
	pinMode(DISPLAY_POWER_PIN, OUTPUT);
	digitalWrite(DISPLAY_POWER_PIN, HIGH);
#else
	Serial2.begin(115200, SERIAL_8N1, 25, 26);
#endif

	Logf("Starting %s", APP_VERSION);
	WiFi.mode(WIFI_AP_STA);

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

	// Load the NTRIP server settings
	_ntripServer0.LoadSettings();
	_ntripServer1.LoadSettings();
	_ntripServer2.LoadSettings();

	_display.Setup();
	Logf("Display type %d setup complete", USER_SETUP_ID);

	_webPortal.Setup();
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
		_lastButtonPress = t;
		Logln("Button 1");
		_display.NextPage();
	}
	if (IsButtonReleased(BUTTON_2, &_button2Current))
	{
		_lastButtonPress = t;
		Logln("Button 2");
		_display.ActionButton();
	}

	// Check if we should turn off the display
	// .. Note : This only work when powered from the GPS unit. WIth ESP32 powered from USB display is always on
#if T_DISPLAY_S3 == true
	digitalWrite(DISPLAY_POWER_PIN, ((t - _lastButtonPress) < 30000) ? HIGH : LOW);
#endif

	// Check for new data GPS serial data
	if (IsWifiConnected())
	{
		_display.SetGpsConnected(_gpsParser.ReadDataFromSerial(Serial2, _ntripServer0, _ntripServer1, _ntripServer2));
		_webPortal.Loop();
	}
	else
	{
		_display.SetGpsConnected(false);
	}

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
		Logf("Wifi Status %d %s", status, WifiStatus(status));
		//	_display.SetWebStatus(status);
		_display.RefreshWiFiState();

		if (status == WL_CONNECTED)
		{
			auto res = _wifiManager.startConfigPortal();
			if (!res)
				Logln("Failed to start config Portal");
			else
				Logln("Config portal started");
		}
	}

	if (status == WL_CONNECTED)
		return true;

	// Start the connection process
	// Logln("E310 - No WIFI");
	unsigned long t = millis();
	unsigned long tDelta = t - _wifiFullResetTime;
	if (tDelta < WIFI_STARTUP_TIMEOUT)
	{
		return false;
	}

	// Reset the WIFI
	//_wifiFullResetTime = t;
	// WiFi.mode(WIFI_STA);
	// wl_status_t beginState = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	// Logf("WiFi Connecting %d %s\r\n", beginState, WifiStatus(beginState));
	//_display.RefreshWiFiState();

	return false;
}

//  Check the Correct TFT Display Type is Selected in the User_Setup.h file
#if USER_SETUP_ID != 206 && USER_SETUP_ID != 25
#error "Error! Please make sure the required LILYGO PCB in .pio\libdeps\lilygo-t-display\TFT_eSPI\User_Setup_Select.h See top of main.cpp for details"
#error "Error! Please make sure the required LILYGO PCB in .pio\libdeps\lilygo-t-display\TFT_eSPI\User_Setup_Select.h See top of main.cpp for details"
#error "Error! Please make sure the required LILYGO PCB in .pio\libdeps\lilygo-t-display\TFT_eSPI\User_Setup_Select.h See top of main.cpp for details"
#endif