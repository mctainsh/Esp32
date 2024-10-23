#pragma once

#include <WiFiManager.h>
#include "HandyString.h"
#include "NTRIPServer.h"
#include "GpsParser.h"

extern WiFiManager _wifiManager;
extern NTRIPServer _ntripServer0;
extern NTRIPServer _ntripServer1;
extern GpsParser _gpsParser;

/// @brief Class manages the web pages displayed in the device.
class WebPortal
{
public:
	void Setup();
	void Loop();

private:
	void OnBindServerCallback();
	void ShowLogHtml();
	void HtmlLog(const char* title, const std::vector<std::string> &log) const;
};

/// @brief Startup the portal
void WebPortal::Setup()
{
	// Setup callbacks
	_wifiManager.setWebServerCallback([this]()
									  { OnBindServerCallback(); });

	// Make access point name
	std::string apName = "RtkSvr-";
	auto macAddress = WiFi.macAddress();
	macAddress.replace(":", "");
	apName += macAddress.c_str();

	Logf("Start AP %s\r\n", apName.c_str());

	_wifiManager.setConfigPortalTimeout(0);
	_wifiManager.setConfigPortalBlocking(false);

	// First parameter is name of access point, second is the password
	//_wifiManager.resetSettings();
	auto res = _wifiManager.autoConnect(apName.c_str(), "Test1234");
	if (!res)
	{
		Logln("WiFi : Failed to connect");
		// ESP.restart();
	}

	Logln("WebPortal setup complete");
}

/// @brief Process the look actions. This is called every loop only if the WiFi connection is available
void WebPortal::Loop()
{
	// Process the WiFi manager (Restart if necessary)
	if (!_wifiManager.getConfigPortalActive())
		_wifiManager.startConfigPortal();
	else
		_wifiManager.process();
}

void WebPortal::HtmlLog(const char* title, const std::vector<std::string> &log) const
{	
	Logf("Show %s log\r\n", title);
	std::string html = "<h1>Log ";
	html += title;
	html += "</h1>";
	html += "<pre>";
	for (const auto& entry : log)
		html += (entry + "\n");
	html += "</pre>";
	_wifiManager.server->send(200, "text/html", html.c_str());
}

void WebPortal::ShowLogHtml()
{
	Logln("ShowLogHtml");
	
}

/// @brief Setup all the URL bindings
void WebPortal::OnBindServerCallback()
{
	Logln("Binding server callback");

	// Simple test page
	_wifiManager.server->on("/TT", HTTP_GET, []()
							{	Logln("TT"); _wifiManager.server->send(200, "text/html", StringPrintf("<h1>HELLO WORLD %d</h1>", millis()).c_str()); });

	_wifiManager.server->on("/gpslog", HTTP_GET, [this]() { HtmlLog("GPS Log",_gpsParser.GetLogHistory()); });
	_wifiManager.server->on("/caster0log", HTTP_GET, [this]() { HtmlLog("GPS Log",_ntripServer0.GetLogHistory()); });
	_wifiManager.server->on("/caster1log", HTTP_GET, [this]() { HtmlLog("GPS Log",_ntripServer1.GetLogHistory()); });
}
