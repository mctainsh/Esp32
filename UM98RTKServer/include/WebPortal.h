#pragma once

#include <WiFiManager.h>
#include "HandyString.h"
#include "NTRIPServer.h"
#include "GpsParser.h"

extern WiFiManager _wifiManager;
extern NTRIPServer _ntripServer0;
extern NTRIPServer _ntripServer1;
extern NTRIPServer _ntripServer2;
extern GpsParser _gpsParser;
extern MyDisplay _display;
extern std::vector<std::string> _mainLog;

/// @brief Class manages the web pages displayed in the device.
class WebPortal
{
public:
	void Setup();
	void Loop();

private:
	void OnBindServerCallback();
	void ShowStatusHtml();
	void HtmlLog(const char *title, const std::vector<std::string> &log) const;
	void OnSaveParamsCallback();

	WiFiManagerParameter *_pCaster0Address;
	WiFiManagerParameter *_pCaster0Port;
	WiFiManagerParameter *_pCaster0Credential;
	WiFiManagerParameter *_pCaster0Password;

	WiFiManagerParameter *_pCaster1Address;
	WiFiManagerParameter *_pCaster1Port;
	WiFiManagerParameter *_pCaster1Credential;
	WiFiManagerParameter *_pCaster1Password;

	WiFiManagerParameter *_pCaster2Address;
	WiFiManagerParameter *_pCaster2Port;
	WiFiManagerParameter *_pCaster2Credential;
	WiFiManagerParameter *_pCaster2Password;
};

/// @brief Startup the portal
void WebPortal::Setup()
{
	// Setup callbacks
	_wifiManager.setWebServerCallback([this]()
									  { OnBindServerCallback(); });
	_wifiManager.setSaveParamsCallback([this]()
									   { OnSaveParamsCallback(); });

	std::string port0String = std::to_string(_ntripServer0.GetPort());
	_pCaster0Address = new WiFiManagerParameter("address0", "Caster 1 address", _ntripServer0.GetAddress().c_str(), 40);
	_pCaster0Port = new WiFiManagerParameter("port0", "Caster 1 port", port0String.c_str(), 6);
	_pCaster0Credential = new WiFiManagerParameter("credential0", "Caster 1 credential ", _ntripServer0.GetCredential().c_str(), 40);
	_pCaster0Password = new WiFiManagerParameter("password0", "Caster 1 password", "", 40);

	std::string port1String = std::to_string(_ntripServer1.GetPort());
	_pCaster1Address = new WiFiManagerParameter("address1", "Caster 2 address", _ntripServer1.GetAddress().c_str(), 40);
	_pCaster1Port = new WiFiManagerParameter("port1", "Caster 2 port", port1String.c_str(), 6);
	_pCaster1Credential = new WiFiManagerParameter("credential1", "Caster 2 credential", _ntripServer1.GetCredential().c_str(), 40);
	_pCaster1Password = new WiFiManagerParameter("password1", "Caster 2 password", "", 40);

	std::string port2String = std::to_string(_ntripServer2.GetPort());
	_pCaster2Address = new WiFiManagerParameter("address2", "Caster 3 address", _ntripServer2.GetAddress().c_str(), 40);
	_pCaster2Port = new WiFiManagerParameter("port2", "Caster 3 port", port2String.c_str(), 6);
	_pCaster2Credential = new WiFiManagerParameter("credential2", "Caster 3 credential", _ntripServer2.GetCredential().c_str(), 40);
	_pCaster2Password = new WiFiManagerParameter("password2", "Caster 3 password", "", 40);

	_wifiManager.addParameter(_pCaster0Address);
	_wifiManager.addParameter(_pCaster0Port);
	_wifiManager.addParameter(_pCaster0Credential);
	_wifiManager.addParameter(_pCaster0Password);

	_wifiManager.addParameter(_pCaster1Address);
	_wifiManager.addParameter(_pCaster1Port);
	_wifiManager.addParameter(_pCaster1Credential);
	_wifiManager.addParameter(_pCaster1Password);

	_wifiManager.addParameter(_pCaster2Address);
	_wifiManager.addParameter(_pCaster2Port);
	_wifiManager.addParameter(_pCaster2Credential);
	_wifiManager.addParameter(_pCaster2Password);

	_wifiManager.setConfigPortalTimeout(0);
	_wifiManager.setConfigPortalBlocking(false);

	// Make access point name
	std::string apName = "RtkSvr-";
	auto macAddress = WiFi.macAddress();
	macAddress.replace(":", "");
	apName += macAddress.c_str();

	Logf("Start AP %s", apName.c_str());

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

void WebPortal::OnSaveParamsCallback()
{
	Logf("SaveParamsCallback");

	_ntripServer0.Save(_pCaster0Address->getValue(), _pCaster0Port->getValue(), _pCaster0Credential->getValue(), _pCaster0Password->getValue());
	_ntripServer1.Save(_pCaster1Address->getValue(), _pCaster1Port->getValue(), _pCaster1Credential->getValue(), _pCaster1Password->getValue());
	_ntripServer2.Save(_pCaster2Address->getValue(), _pCaster2Port->getValue(), _pCaster2Credential->getValue(), _pCaster2Password->getValue());

	ESP.restart();
}

void WebPortal::HtmlLog(const char *title, const std::vector<std::string> &log) const
{
	Logf("Show %s", title);
	std::string html = "<h3>Log ";
	html += title;
	html += "</h3>";
	html += "<pre>";
	for (const auto &entry : log)
		html += (entry + "\n");
	html += "</pre>";
	_wifiManager.server->send(200, "text/html", html.c_str());
}

std::string I(int n)
{
	std::string repeatedString;
	for (int i = 0; i < n; ++i)
		repeatedString += "&nbsp; &nbsp; ";
	return repeatedString;
}

std::string TableRow(int indent, const std::string &name, const char *value)
{
	return "<tr><td>" + I(indent) + name + "</td><td>" + value + "</td></tr>";
}
std::string TableRow(int indent, const std::string &name, const std::string &value)
{
	return "<tr><td>" + I(indent) + name + "</td><td>" + value + "</td></tr>";
}

std::string TableRow(int indent, const std::string &name, int32_t value)
{
	return TableRow(indent, name, StringPrintf("%d", value));
}

void WebPortal::ShowStatusHtml()
{
	Logln("ShowStatusHtml");
	std::string html = "<h3>System Status</h3>";
	html += "<table>";
	html += TableRow(0, "General", "");
	html += TableRow(1, "Version", APP_VERSION);
	html += TableRow(1, "Free Heap", ESP.getFreeHeap());
	html += TableRow(1, "Free Sketch Space", ESP.getFreeSketchSpace());
	html += TableRow(0, "GPS", "");
	html += TableRow(1, "Device type", _gpsParser.GetDeviceType());
	html += TableRow(1, "Device firmware", _gpsParser.GetDeviceFirmware());
	html += TableRow(1, "Device serial #", _gpsParser.GetDeviceSerial());

	int32_t resetCount, reinitialize, packetCount;
	_display.GetGpsStats(resetCount, reinitialize, packetCount);
	html += TableRow(1, "Reset count", resetCount);
	html += TableRow(1, "Reinitialize count", reinitialize);
	html += TableRow(1, "Packet count", packetCount);

	//	html += TableRow("", );
	html += "</table>";
	_wifiManager.server->send(200, "text/html", html.c_str());
}

/// @brief Setup all the URL bindings
void WebPortal::OnBindServerCallback()
{
	Logln("Binding server callback");

	// Simple test page
	_wifiManager.server->on("/TT", HTTP_GET, []()
							{	Logln("TT"); _wifiManager.server->send(200, "text/html", StringPrintf("<h1>HELLO WORLD %d</h1>", millis()).c_str()); });

	_wifiManager.server->on("/log", HTTP_GET, [this]()
							{ HtmlLog("System log", _mainLog); });
	_wifiManager.server->on("/gpslog", HTTP_GET, [this]()
							{ HtmlLog("GPS log", _gpsParser.GetLogHistory()); });
	_wifiManager.server->on("/caster1log", HTTP_GET, [this]()
							{ HtmlLog("Caster 1 log", _ntripServer0.GetLogHistory()); });
	_wifiManager.server->on("/caster2log", HTTP_GET, [this]()
							{ HtmlLog("Caster 2 log", _ntripServer1.GetLogHistory()); });
	_wifiManager.server->on("/status", HTTP_GET, [this]()
							{ ShowStatusHtml(); });
}