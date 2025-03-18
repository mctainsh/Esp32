#include <Arduino.h>
///////////////////////////////////////////////////////////////////////////////
//	  #########################################################################
//	  ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
//	  #########################################################################
//
//	To use the TTGO T-Display with the TFT_eSPI library, you need to make the following changes to
//	the User_Setup.h file in the library.
//		.pio\libdeps\lilygo-t-display\TFT_eSPI\User_Setup_Select.h
//
//	1. Comment out the default setup file
//		//#include <User_Setup.h>           // Default setup is root library folder
//	2. Uncomment the TTGO T-Display setup file
//		#include <User_Setups/Setup25_TTGO_T_Display.h>    // Setup file for ESP32 and TTGO T-Display ST7789V SPI bus TFT
//	3. Add the following line to the start of the file
//		#define DISABLE_ALL_LIBRARY_WARNINGS
//
// NOTE: When Using rtk2go.com
//		Username should be a valid email address.
//		Password “none” if your device requires that a password be entered.
///////////////////////////////////////////////////////////////////////////////

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <iostream>
#include <sstream>
#include <string>

#include "Global.h"

#include "HandyString.h"
#include "MyDisplay.h"
#include "GpsParser.h"
#include "MyFiles.h"
#include "NTRIPClient.h"
#include "ButtonInterrupt.h"
#include "WebPortal.h"
#include <WifiBusyTask.h>

WiFiManager _wifiManager;
WebPortal _webPortal;

MyFiles _myFiles;
NTRIPClient _ntripClient;
MyDisplay _display([]()
				   { _ntripClient.DisplaySettings(); });
GpsParser _gpsParser(_display);

ButtonInterrupt _button1(BUTTON_1);
ButtonInterrupt _button2(BUTTON_2);

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
String MakeHostName();

///////////////////////////////////////////////////////////////////////////////
// Startup logging to screen and serial
void LogI(TFT_eSPI &tft, const char *str)
{
	Serial.print(str);
	tft.print(str);
}

///////////////////////////////////////////////////////////////////////////////
// Setup
void setup(void)
{
	Serial.begin(115200);
	Serial.println("Starting");

	// Setup temporary startup display
	auto tft = TFT_eSPI();
	tft.init();
	tft.setRotation(3);
	tft.fillScreen(TFT_GREEN);
	tft.setTextColor(TFT_BLACK, TFT_GREEN);
	tft.setTextFont(2);

	LogI(tft, StringPrintf("Starting %s\r\n", APP_VERSION).c_str());
	LogI(tft, "GPS, ");
#if T_DISPLAY_S3 == true
	Serial2.begin(115200, SERIAL_8N1, 12, 13);
	// Turn on display power for the TTGO T-Display-S3 (Needed for battery operation or if powered from 5V pin)
	pinMode(DISPLAY_POWER_PIN, OUTPUT);
	digitalWrite(DISPLAY_POWER_PIN, HIGH);
#else
	Serial2.begin(115200, SERIAL_8N1, 25, 26);
#endif
	LogI(tft, "Btn interrupts, ");
	_button1.AttachInterrupts([]() IRAM_ATTR
							  { _button1.OnFallingIsr(); }, []() IRAM_ATTR
							  { _button1.OnRaisingIsr(); });
	_button2.AttachInterrupts([]() IRAM_ATTR
							  { _button2.OnFallingIsr(); }, []() IRAM_ATTR
							  { _button2.OnRaisingIsr(); });

	// Verify file IO
	LogI(tft, "Check file access, ");
	SetupLog();
	if (_myFiles.Setup())
	{
		//_myFiles.WriteFile("/hello.txt", "Hello ");
		//_myFiles.AppendFile("/hello.txt", "World!\r\n");

		// std::string response;
		//_myFiles.ReadFile("/hello.txt", response);
		// Logln(response.c_str());
		_ntripClient.LoadSettings();

		_gpsParser.GetGpsSender().LoadSettings();
	}

	// Setup the WIFI
	LogI(tft, "WIFI Setup\r\n");
	auto hostname = MakeHostName();
	WiFi.setHostname(hostname.c_str());
	_wifiManager.setHostname(hostname.c_str());
	auto hostNameMsg = StringPrintf("Host : %s", WiFi.getHostname()).c_str();
	Logln(hostNameMsg);
	LogI(tft, hostNameMsg);
	LogI(tft, "\r\n");
	LogI(tft, "IP : 192.168.4.1\r\n");
	// Reset Wifi Setup if needed (Do tis to clear out old wifi credentials)
	//_wifiManager.erase();

	// Setup display
	LogI(tft, "Setup display\r\n");
	_display.Setup();
	_display.RefreshWiFiState();
	
	// Start connecting loop
	WifiBusyTask wifiBusy(_display);
	while (WiFi.status() != WL_CONNECTED)
	{
		int wifiTimeoutSeconds = wifiBusy.StartCountDown();
		_wifiManager.setConfigPortalTimeout(wifiTimeoutSeconds);		
		_wifiManager.autoConnect(WiFi.getHostname(), AP_PASSWORD);

		_display.SetCell(_wifiManager.getWiFiSSID(true).c_str(), 0, 2);	// Only works after first call
	}

	// Connected
	_display.SetCell("", 0, 4);

	Logln("Setup Web Portal");
	_webPortal.Setup(hostname.c_str());

	Logf("Startup Complete. Connected to %s", WiFi.SSID().c_str());
}

///////////////////////////////////////////////////////////////////////////////
// Loop here
void loop()
{
	// Check if we should turn off the display
	// .. Note : This only work when powered from the GPS unit. WIth ESP32 powered from USB display is always on
#if T_DISPLAY_S3 == true
	// digitalWrite(DISPLAY_POWER_PIN, ((t - _lastButtonPress) < 30000) ? HIGH : LOW);
#endif

	// Trigger something every second
	int t = millis();
	_loopPersSecondCount++;
	if ((t - _loopWaitTime) > 1000)
	{
		_loopWaitTime = t;
		_display.SetLoopsPerSecond(_loopPersSecondCount);
		_loopPersSecondCount = 0;
	}

	// Check for push buttons
	if (_button1.WasPressed())
	{
		Logln("Button 1");
		_display.NextPage();
	}
	if (_button2.WasPressed())
	{
		Logln("Button 2");
		if( _display.ActionButton() == MyDisplay::Actions::RESET_GPS)
			_gpsParser.GetCommandQueue().StartResetProcess();
	}

	// Check for new data GPS serial data
	_display.SetGpsConnected(_gpsParser.ReadDataFromSerial(Serial2));

	// WIFI related functions
	if (IsWifiConnected())
	{
		// Check for new RTK corrections if we have a location already
		if (_display.HasLocation())
			_ntripClient.Loop();
	}

	// Keep web portal awake
	_webPortal.Loop();

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
		_display.RefreshWiFiState();
		if (status == WL_CONNECTED)
		{
			_display.SetRtkStatus("GPS Pending");
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

		// if( status != WL_DISCONNECTED)
		//	Logln(stateTitle.c_str());
		//_display.SetWebStatus(stateTitle.c_str());

		// Create a string of dots to show progress. 1 dot per second
		std::string dotsStr(tDelta / 1000, '.');
		_display.SetRtkStatus(dotsStr.c_str());

		return false;
	}

	// Reset the WIFI
	_wifiFullResetTime = t;
	//	WiFi.mode(WIFI_STA);
	//	wl_status_t beginState = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	//	Serial.printf("WiFi Connecting %d %s\r\n", beginState, WifiStatus(beginState));

	return false;
}

///////////////////////////////////////////////////////////////////////////////
/// @brief Maker a unique host name based on the MAC address with Rtk prefix
String MakeHostName()
{
	// return WiFi.getHostname();
	auto mac = WiFi.macAddress();
	mac.replace(":", "");
	return "RT_" + mac;
}

//  Check the Correct TFT Display Type is Selected in the User_Setup.h file
#if USER_SETUP_ID != 206 && USER_SETUP_ID != 25
#error "Error! Please make sure the required LILYGO PCB in .pio\libdeps\lilygo-t-display\TFT_eSPI\User_Setup_Select.h See top of main.cpp for details"
#error "Error! Please make sure the required LILYGO PCB in .pio\libdeps\lilygo-t-display\TFT_eSPI\User_Setup_Select.h See top of main.cpp for details"
#error "Error! Please make sure the required LILYGO PCB in .pio\libdeps\lilygo-t-display\TFT_eSPI\User_Setup_Select.h See top of main.cpp for details"
#endif