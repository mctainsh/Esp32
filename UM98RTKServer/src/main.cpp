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
#include <WiFiManager.h>  //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <mDNS.h>		  // Setup *.local domain name
#include "esp_task_wdt.h" // Include for esp_task_wdt_delete

void SaveBaseLocation(std::string newBaseLocation);

#include "Global.h"
#include "HandyLog.h"
#include "HandyString.h"
#include "MyDisplay.h"
#include "GpsParser.h"
#include "NTRIPServer.h"
#include "MyFiles.h"
#include <Web\WebPortal.h>
#include "WiFiEvents.h"
#include "History.h"

WiFiManager _wifiManager;

unsigned long _slowLoopWaitTime = 0;  // Time of last 10 second
unsigned long _fastLoopWaitTime = 0;  // Time of last second
unsigned long _lastWiFiConnected = 0; // Time of last WiFi connection
unsigned long _lastLoopTime = 0;	  // Time of last loop
int _loopPersSecondCount = 0;		  // Number of times the main loops runs in a second
unsigned long _lastButtonPress = 0;	  // Time of last button press to turn off display on T-Display-S3
History _history;					  // Temperature history

WebPortal _webPortal;

uint8_t _button1Current = HIGH; // Top button on left
uint8_t _button2Current = HIGH; // Bottom button when

MyFiles _myFiles;
MyDisplay _display;
GpsParser _gpsParser(_display);
NTRIPServer _ntripServer0(0);
NTRIPServer _ntripServer1(1);
NTRIPServer _ntripServer2(2);
std::string _baseLocation = "";
std::string _mdnsHostName;
HandyTime _handyTime;
bool _slowLoopFirstHalf;

// WiFi monitoring states
#define WIFI_RESTART_TIMEOUT 120000
wl_status_t _lastWifiStatus = wl_status_t::WL_NO_SHIELD;

bool IsButtonReleased(uint8_t button, uint8_t *pCurrent);
bool IsWifiConnected();

///////////////////////////////////////////////////////////////////////////////
// Setup
void setup(void)
{
	perror("RTL Server - Starting");
	perror(APP_VERSION);

	// Disable the watchdog timer
	esp_task_wdt_delete(NULL); // Delete the default task watchdog
	esp_task_wdt_deinit();	   // Deinitialize the watchdog timer

	// Setup temporary startup display
	auto tft = TFT_eSPI();
	tft.init();
	tft.setRotation(TFT_ROTATION); // One 1 buttons on right, 3 Buttons on left
	tft.fillScreen(TFT_GREEN);
	tft.setTextColor(TFT_BLACK, TFT_GREEN);
	tft.setTextFont(2);
	tft.printf("Starting %s. Cores %d\r\n", APP_VERSION, configNUM_CORES);

	SetupLog(); // Call this before any logging

	// Record reset reason
	esp_reset_reason_t reason = esp_reset_reason();
	Logf("RESET Reason : %d - %s", reason, ResetReasonText(reason)); // ESP_RST_TASK_WDT means task watchdog triggered

	// No logging before here
	Serial.begin(115200); // Using perror() instead
	Logf("Starting %s. Cores:%d", APP_VERSION, configNUM_CORES);

	// Setup the serial buffer for the GPS port
	Logf("GPS Buffer size %d", Serial2.setRxBufferSize(GPS_BUFFER_SIZE));

	tft.println("Enable Display pins");
#ifdef T_DISPLAY_S3
	// Turn on display power for the TTGO T-Display-S3 (Needed for battery operation or if powered from 5V pin)
	pinMode(DISPLAY_POWER_PIN, OUTPUT);
	digitalWrite(DISPLAY_POWER_PIN, HIGH);
#endif

	Logln("Enable WIFI");
	tft.println("Enable WIFI");
	SetupWiFiEvents();

	Logln("Enable Buttons");
	tft.println("Enable Buttons");
	pinMode(BUTTON_1, INPUT_PULLUP);
	pinMode(BUTTON_2, INPUT_PULLUP);

	// Verify file IO (This can take up tpo 60s is SPIFFs not initialised)
	tft.println("Setup SPIFFS");
	tft.println("This can take up to 60 seconds ...");
	if (_myFiles.Setup())
		Logln("Test file IO OK");
	else
		Logln("E100 - File IO failed");
	_myFiles.LoadString(_baseLocation, BASE_LOCATION_FILENAME);

	// Load the NTRIP server settings
	tft.println("Setup NTRIP Connections");
	_ntripServer0.LoadSettings();
	_ntripServer1.LoadSettings();
	_ntripServer2.LoadSettings();
	_gpsParser.Setup(&_ntripServer0, &_ntripServer1, &_ntripServer2);

	_display.Setup();
#ifdef USER_SETUP_ID
	Logf("Display type %d", USER_SETUP_ID);
#endif

	// Reset Wifi Setup if needed (Do tis to clear out old wifi credentials)
	//_wifiManager.erase();

	// Setup host name to have RTK_ prefix
	WiFi.setHostname(MakeHostName().c_str());
	_display.RefreshScreen();

	// // Block here till we have WiFi credentials (good or bad)
	// Logf("Start listening on %s", MakeHostName().c_str());

	// const int wifiTimeoutSeconds = 120;
	// WifiBusyTask wifiBusy(_display);
	// _wifiManager.setConfigPortalTimeout(wifiTimeoutSeconds);
	// while (WiFi.status() != WL_CONNECTED)
	// {
	// 	Logf("Try WIFI Connection on %s", MakeHostName().c_str());
	// 	wifiBusy.StartCountDown(wifiTimeoutSeconds);
	// 	_wifiManager.autoConnect(WiFi.getHostname(), AP_PASSWORD);
	// 	// ESP.restart();
	// 	// delay(1000);
	// }

	// Setup the web portal
	//	_webPortal.Setup();
	//	_handyTime.EnableTimeSync(_myFiles.LoadString(TIMEZONE_MINUTES));
	//	Logln("Time sync enabled");

	// Setup the MDNS responder
	// .. This will allow us to access the server using http://RtkServer.local
	Logln("MDNS Read");
	_myFiles.LoadString(_mdnsHostName, MDNS_HOST_FILENAME);
	if (_mdnsHostName.empty())
		_mdnsHostName = "RtkServer"; // Default hostname for mDNS

	Logf("MDNS Setup %s", _mdnsHostName.c_str());
	mdns_init();
	mdns_hostname_set(_mdnsHostName.c_str());
	mdns_instance_name_set(_mdnsHostName.c_str());
	Serial.printf("MDNS responder started at http://%s.local\n", _mdnsHostName.c_str());
	_display.RefreshScreen();

	// Dump the file structure
	_myFiles.StartupComplete();

	Logln("Setup complete");
	Serial.println(" ========================== Setup done ========================== ");
	_webPortal.SetupWiFi();

	// Start the watch dog timer
	esp_task_wdt_init(120, true); // 60 seconds timeout, panic on timeout
	esp_task_wdt_add(NULL);		  // Add the current task to the watchdog
	esp_task_wdt_reset();
}

///////////////////////////////////////////////////////////////////////////////
// Loop here
void loop()
{
	// Trigger something 1 every seconds
	int t = millis();
	_loopPersSecondCount++;
	if ((t - _fastLoopWaitTime) > 1000)
	{
		// Refresh RTK Display
		for (int i = 0; i < RTK_SERVERS; i++)
			_display.RefreshRtk(i);
		_fastLoopWaitTime = t;
		_loopPersSecondCount = 0;
		_display.DisplayTime(t);

		// Update WiFi timeout if not connected
		if (WiFi.status() != WL_CONNECTED)
			_display.RefreshWiFiState((WIFI_RESTART_TIMEOUT - (t - _lastWiFiConnected)) / 1000.0);
	}

	// Is this the first half of the long loop?
	_slowLoopFirstHalf = ((t - _slowLoopWaitTime) < 5000);

	// Run every 10 seconds
	if ((t - _slowLoopWaitTime) > 10000)
	{
		// Reset the watchdog timer
		esp_task_wdt_reset();

		// Check memory pressure
		auto free = ESP.getFreeHeap();
		auto total = ESP.getHeapSize();

		auto temperature = _history.CheckTemperatureLoop();

		// Update the loop performance counter
		Serial.printf("%s Loop %d G:%ld Heap:%d%% %.1f°C %s\n",
					  _handyTime.LongString().c_str(),
					  _loopPersSecondCount,
					  _gpsParser.GetGpsBytesRec(),
					  (int)(100.0 * free / total),
					  temperature,
					  WiFi.localIP().toString().c_str());

		// Disable Access point mode
		// if (WiFi.getMode() != WIFI_STA && WiFi.status() != WL_DISCONNECTED)
		// {
		// 	Logln("W105 - WiFi mode is not WIFI_STA, resetting");
		// 	WiFi.softAPdisconnect(false);
		// 	//_wifiManager.setConfigPortalTimeout(60);
		// 	Logln("Set WIFI_STA mode");
		// 	WiFi.mode(WIFI_STA);
		// }
		// Disable Access point mode
		if (WiFi.getMode() != WIFI_STA && WiFi.status() == WL_CONNECTED)
		{
			Logln("W105 - WiFi mode is not WIFI_STA, resetting");
			WiFi.softAPdisconnect(false);
			//_wifiManager.setConfigPortalTimeout(60);
			Logln("Set WIFI_STA mode");
			WiFi.mode(WIFI_STA);
			//_swipePageGps.RefreshData();
			//_lvCore.UpdateWiFiIndicator();
			_display.RefreshWiFiState();
		}

		// Performance text
		std::string perfText = StringPrintf("%d%% %.0fC %ddBm",
											(int)(100.0 * free / total),
											temperature,
											WiFi.RSSI());
		_display.SetPerformance(perfText);
		_slowLoopWaitTime = t;
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
#ifdef T_DISPLAY_S3
	digitalWrite(DISPLAY_POWER_PIN, ((t - _lastButtonPress) < 30000) ? HIGH : LOW);
#endif

	// Check for new data GPS serial data
	if (IsWifiConnected())
		_display.SetGpsConnected(_gpsParser.ReadDataFromSerial(Serial2));
	else
		_display.SetGpsConnected(false);
	_webPortal.Loop();

	// Update animations
	_display.Animate();
}

//////////////////////////////////////////////////////////////////////////////
// Read the base location from the disk
void SaveBaseLocation(std::string newBaseLocation)
{
	_baseLocation = newBaseLocation;
	_myFiles.WriteFile(BASE_LOCATION_FILENAME, newBaseLocation.c_str());
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
		//_wifiFullResetTime = millis();

		if (status == WL_CONNECTED)
		{
			Logf("IP:%s Host:%s", WiFi.localIP().toString().c_str(), _mdnsHostName.c_str());
			// Setup the access point to prevend device getting stuck on a nearby network
			// auto res = _wifiManager.startConfigPortal(WiFi.getHostname(), AP_PASSWORD);
			// if (!res)
			//	Logln("Failed to start config Portal (Maybe cos non-blocked)");
			// else
			//	Logln("Config portal started");

			// TODO : Set the mDNS host name
			// TODO : Get latest time from the RTC
			// TODO : Save the SSID name
			_webPortal.OnConnected();
		}
	}

	if (status == WL_CONNECTED)
	{
		_lastWiFiConnected = millis();
		return true;
	}

	// if (status != WL_DISCONNECTED && _webPortal.GetConnectCount() > 0)
	//		return false;

	// If we have been disconnected, we will try to reconnect
	// if( status == WL_DISCONNECTED && ( millis() - _lastWiFiConnected ) > 120000 )
	if ((millis() - _lastWiFiConnected) > WIFI_RESTART_TIMEOUT)
	{
		Logln("E107 - WIFI connect timeout");
		_display.RefreshWiFiState();
		_lastWiFiConnected = millis();
		_webPortal.SetupWiFi(); // Restart the web portal
	}

	// Block here until we are connected again
	//	_webPortal.Setup();
	return false;

	// // Reset the WIFI if Disconnected (This seems to be unrecoverable)
	// auto delay = millis() - _wifiFullResetTime;
	// auto message = StringPrintf("[X:%d] FORCE RECONNECT", delay / 1000);
	// Serial.println(message.c_str());
	// _display.SetCell(message, DISPLAY_PAGE, DISPLAY_ROW);

	// // Start the connection process
	// // Logln("E310 - No WIFI");
	// unsigned long t = millis();
	// if (delay < WIFI_STARTUP_TIMEOUT)
	// 	return false;

	// //_wifiManager.resetSettings();

	// Logln("E107 - Try resetting WIfi");
	// _wifiFullResetTime = t;

	// // We will not block here until the WIFI is connected
	// _wifiManager.setConfigPortalBlocking(false);
	// _wifiManager.startConfigPortal(WiFi.getHostname(), AP_PASSWORD);
	// // WiFi.mode(WIFI_STA);
	// // wl_status_t beginState = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	// // Logf("WiFi Connecting %d %s\r\n", beginState, WifiStatus(beginState));
	// _display.RefreshWiFiState();

	return false;
}

///////////////////////////////////////////////////////////////////////////////
// This function is called when the task watchdog timer is triggered
extern "C" void esp_task_wdt_isr_user_handler(void)
{
	Logln("⚠️ Task Watchdog Timer triggered!");
}

//  Check the Correct TFT Display Type is Selected in the User_Setup.h file
#ifndef USER_SETUP_LOADED
#if USER_SETUP_ID != 206 && USER_SETUP_ID != 25
#error "Error! Please make sure the required LILYGO PCB in .pio\libdeps\lilygo-t-display\TFT_eSPI\User_Setup_Select.h See top of main.cpp for details"
#error "Error! Please make sure the required LILYGO PCB in .pio\libdeps\lilygo-t-display\TFT_eSPI\User_Setup_Select.h See top of main.cpp for details"
#error "Error! Please make sure the required LILYGO PCB in .pio\libdeps\lilygo-t-display\TFT_eSPI\User_Setup_Select.h See top of main.cpp for details"
#endif
#endif