
#ifndef GPSSENDER_H
#define GPSSENDER_H

#include <WiFi.h>
#include <HTTPClient.h>

///////////////////////////////////////////////////////////////////////////////
// Send GPS data in FRI format
// +RESP:GTFRI,C30203,860201061332360,,0,0,1,0,0.0,,,153.349838,-27.838360,20241011180939,,,,,,100,20241012042623,2044$
class GpsSender
{
public:
	GpsSender(const char *ssid, const char *password, const char *serverUrl);
	void connectWiFi();
	void sendGpsData(float latitude, float longitude);

private:
	const char *ssid;
	const char *password;
	const char *serverUrl;
};

GpsSender::GpsSender(const char *ssid, const char *password, const char *serverUrl)
	: ssid(ssid), password(password), serverUrl(serverUrl) {}

void GpsSender::connectWiFi()
{
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(1000);
	}
}

void GpsSender::sendGpsData(float latitude, float longitude)
{
	if (WiFi.status() == WL_CONNECTED)
	{
		HTTPClient http;
		http.begin(serverUrl);
		http.addHeader("Content-Type", "application/json");

		String jsonPayload = "{\"latitude\":" + String(latitude, 6) + ",\"longitude\":" + String(longitude, 6) + "}";
		int httpResponseCode = http.POST(jsonPayload);

		http.end();
	}
}

#endif // GPSSENDER_H