#pragma once

#include "HandyString.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include "MyDisplay.h"
#include "MyFiles.h"

extern MyFiles _myFiles;

///////////////////////////////////////////////////////////////////////////////
// Send GPS data in FRI format
// This will make it mostly compatible with the GL320MG with some extra fields
// 	+RESP:GTFRI,C30203,860201061332360,,0,0,1,0,0.0,,,153.349838,-27.838360,20241011180939,,,,,,100,20241012042623,2044$
// Index 	Parameter 			Description
// 	1  		Product version		C30203
// 	2  		IMEI				800010002000310
// 	3 		Device Name			UM982
// 	4 		Report ID			0 - 4
// 	5 		Report Type			0|1|16|17 (Always one for Scheduled Report)
//  6 		Number				1 (Point in the set "Alway one for me")
//  7 		GPS Accuracy		0 - 50
//  8 		Speed				0.0 - 999.9(km/h)
//  9 		Azimuth				0 - 359
//  10 		Altitude			(-)XXXXX.X(m)
//  11 		Longitude			(-)XXX.XXXXXX
//  12 		Latitude			(-)XX.XXXXXX
//  13 		GPS UTC Time		YYYYMMDDHHMMSS
//  14 		MCC					0XXX (Country Code replaced with NUmber of Satellites)
//  15 		MNC					0XXX (Network Code replaced with FixMode)
//  16 		LAC					(HEX)
//  17 		Cell ID				(HEX)
//  18 		ODO Mileage			0 (Unused)
//  19 		Battery Percentage	0 - 100 (Unused)
//  20 		Send Time			YYYYMMDDHHMMSS (Same as GPS time)
//  21 		Count Number		(HEX) (Unused)
//  22 		Tail Character		$
class GpsSender
{
public:
	GpsSender(MyDisplay &display);
	void LoadSettings();
	void Save(const char *url, const char *deviceId);
	void SendHttpData(double latitude, double longitude, double altitude, int satellites, int fixMode);
	void SendTcpData(const char *ip, uint16_t port, const String &text);

	inline const std::string GetUrl() const { return _sUrl; }
	inline const std::string GetDeviceId() const { return _sDeviceId; }

private:
	MyDisplay &_display;
	unsigned long _lastSend;
	std::string _sUrl;
	std::string _sDeviceId;
	int _sendDelay = 10000;	// Period to wait before resending data
	const char *SETTING_FILE = "/GpsSenderSettings.txt";
};

///////////////////////////////////////////////////////////////////////////////
// Constructor
// LOad the settings
GpsSender::GpsSender(MyDisplay &display) : _display(display)
{
}

void GpsSender::LoadSettings()
{
	// Load default settings
	_sUrl = "https://www.yoursite.com/api/weatherforecast/SaveLocation";
	_sDeviceId = "5558802";

	// Read the server settings from the config file
	std::string llText;
	if (_myFiles.ReadFile(SETTING_FILE, llText))
	{
		Logf(" - Read config '%s'", llText.c_str());
		auto parts = Split(llText, "\n");
		if (parts.size() > 1)
		{
			_sUrl = parts[0];
			_sDeviceId = parts[1];
			Logf(" - Recovered\r\n\t URL      : %s\r\n\t DeviceId : %s", _sUrl.c_str(), _sDeviceId.c_str());
		}
		else
		{
			Logf(" - E341 - Cannot read saved Server settings %s", llText.c_str());
		}
	}
	else
	{
		Logf(" - E342 - Cannot read saved Server setting %s", SETTING_FILE);
	}
}

//////////////////////////////////////////////////////////////////////////////
// Save the setting to the file
void GpsSender::Save(const char *url, const char *deviceId)
{
	std::string llText = StringPrintf("%s\n%s", url, deviceId);
	_myFiles.WriteFile(SETTING_FILE, llText.c_str());
}

void GpsSender::SendHttpData(double latitude, double longitude, double altitude, int satellites, int fixMode)
{
	//Logf("Wait %d < %d\r\n", millis() - _lastSend, _sendDelay);
	// Is it time to send?
	if (millis() - _lastSend < _sendDelay)
		return;

	_lastSend = millis();
	_sendDelay = 60000; // Wait a minute before trying again if an error occurs

	// Changed to send to @Track server
	// .. +RESP:GTFRI,F50904,015181003555787,,0,0,1,1,0.0,0,106.2,153.057701,-27.594989,20220513005727,0505,0001,7028,08C8C20D,,97,20220513005750,07E0$
	//auto line = StringPrintf("+RESP:GTFRI,F50904,888881111000787,,0,0,1,1,0.0,0,106.2,%.6lf,%.6lf,yyyyMMddHHmmss,0505,0001,7028,08C8C20D,,97,yyyyMMddHHmmss,07E0$\r\n", longitude, latitude);
	//SendTcpData("some address", 22392, line.c_str());
	//return;

	// Build and send
	//std::string httpStr = StringPrintf("%s/%.9lf/%.9lf/%lf/%d/%d", _sUrl.c_str(), longitude, latitude, altitude, satellites, fixMode);
	std::string httpStr = StringPrintf("%s/%.9lf/%.9lf/%lf/%d/%d/%s", _sUrl.c_str(), longitude, latitude, altitude, satellites, fixMode, _sDeviceId.c_str());
	if (WiFi.status() == WL_CONNECTED)
	{
		HTTPClient http;
		http.begin(httpStr.c_str()); //Specify the URL
		int httpCode = http.GET();
		if (httpCode == 200)
		{
			String payload = http.getString();
			if (payload.startsWith("OK-"))
			{
				Logln(payload.c_str());
				_lastSend = millis();
				_display.IncrementSendGood(httpCode);
				_sendDelay = 500;
				return;
			}
			Logln(httpStr.c_str());
			Logf("Http code:%d",httpCode);
			Logln(payload.c_str());
		}
		else
		{
			Logf("Error %d on HTTP in %s request", httpCode, httpStr.c_str());
		}
		http.end(); // Free the resources
		_display.IncrementSendBad(httpCode);
	}
}

void GpsSender::SendTcpData(const char *ip, uint16_t port, const String &text)
{
	WiFiClient client;
	if (client.connect(ip, port))
	{
		client.print(text);
		client.stop();
	}
	else
	{
		Logf("Failed to connect to server %s %d", ip, port);
	}
}