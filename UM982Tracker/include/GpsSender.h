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
	uint16_t _lastSend;
	std::string _sUrl;
	std::string _sDeviceId;
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
	_sDeviceId = "McTainsh001";

	// Read the server settings from the config file
	std::string llText;
	if (_myFiles.ReadFile(SETTING_FILE, llText))
	{
		Serial.printf(" - Read config '%s'\r\n", llText.c_str());
		auto parts = Split(llText, "\n");
		if (parts.size() > 1)
		{
			_sUrl = parts[0];
			_sDeviceId = parts[1];
			Serial.printf(" - Recovered\r\n\t URL      : %s\r\n\t DeviceId : %s\r\n", _sUrl.c_str(), _sDeviceId.c_str());
		}
		else
		{
			Serial.printf(" - E341 - Cannot read saved Server settings %s\r\n", llText.c_str());
		}
	}
	else
	{
		Serial.printf(" - E342 - Cannot read saved Server setting %s\r\n", SETTING_FILE);
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
	// Is it time to send?
	if (millis() - _lastSend < 4500)
		return;

	// Build and send
	std::string httpStr = StringPrintf("%s/%.9lf/%.9lf/%lf/%d/%d", _sUrl.c_str(), longitude, latitude, altitude, satellites, fixMode);
	const char *httpBuff = httpStr.c_str();
	if (WiFi.status() == WL_CONNECTED)
	{
		HTTPClient http;
		http.begin(httpBuff);
		int httpCode = http.GET();
		if (httpCode == 200)
		{
			String payload = http.getString();
			if (payload.startsWith("OK-"))
			{
				_lastSend = millis();
				_display.IncrementSendGood(httpCode);
				return;
			}
			Serial.println(httpBuff);
			Serial.println(httpCode);
			Serial.println(payload);
		}
		else
		{
			Serial.printf("Error %d on HTTP in %s request\r\n", httpCode, httpBuff);
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
		Serial.printf("Failed to connect to server %s %d\r\n", ip, port);
	}
}