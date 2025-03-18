#pragma once

#include <WiFiManager.h>
#include "HandyString.h"
#include "GpsParser.h"
#include "Global.h"
#include "NTRIPClient.h"

extern WiFiManager _wifiManager;
extern GpsParser _gpsParser;
extern MyDisplay _display;
extern NTRIPClient _ntripClient;
extern std::vector<std::string> _mainLog;

/// @brief Class manages the web pages displayed in the device.
class WebPortal
{
public:
	void Setup(const char *hostname);
	void Loop();

private:
	void OnBindServerCallback();
	void IndexHtml();
	void ShowStatusHtml();
	void HtmlLog(const char *title, const std::vector<std::string> &log) const;
	void OnSaveParamsCallback();

	int _loops = 0;

	WiFiManagerParameter *_pNtripAddress;
	WiFiManagerParameter *_pNtripPort;
	WiFiManagerParameter *_pNtripMountPoint;
	WiFiManagerParameter *_pNtripUsername;
	WiFiManagerParameter *_pNtripPassword;

	WiFiManagerParameter *_pGpsSenderUrl;
	WiFiManagerParameter *_pGpsSenderDeviceId;
};

/// @brief Startup the portal
void WebPortal::Setup(const char *hostname)
{
	// Setup callbacks
	_wifiManager.setWebServerCallback(std::bind(&WebPortal::OnBindServerCallback, this));
	_wifiManager.setSaveParamsCallback([this]()
									   { OnSaveParamsCallback(); });

	_pNtripAddress = new WiFiManagerParameter("NtripAddress", "NTRIP Address (ie ntrip.data.gnss.ga.gov.au)", _ntripClient.GetAddress().c_str(), 52);
	_pNtripPort = new WiFiManagerParameter("NTripPort", "NTRIP port (0 = off)", _ntripClient.GetPort().c_str(), 6);
	_pNtripMountPoint = new WiFiManagerParameter("NtripMountPoint", "NTRIP Mount Point", _ntripClient.GetMountPoint().c_str(), 48);
	_pNtripUsername = new WiFiManagerParameter("NtripUsername", "NTRIP Username", _ntripClient.GetUsername().c_str(), 48);
	_pNtripPassword = new WiFiManagerParameter("NtripPassword", "NTRIP Password", _ntripClient.GetPassword().c_str(), 48);

	_pGpsSenderUrl = new WiFiManagerParameter("GpsSenderUrl", "Tracking Server URL", _gpsParser.GetGpsSender().GetUrl().c_str(), 128);
	_pGpsSenderDeviceId = new WiFiManagerParameter("GpsSenderDeviceId", "Tracker Device ID", _gpsParser.GetGpsSender().GetDeviceId().c_str(), 48);

	_wifiManager.addParameter(_pNtripAddress);
	_wifiManager.addParameter(_pNtripPort);
	_wifiManager.addParameter(_pNtripMountPoint);
	_wifiManager.addParameter(_pNtripUsername);
	_wifiManager.addParameter(_pNtripPassword);

	_wifiManager.addParameter(_pGpsSenderUrl);
	_wifiManager.addParameter(_pGpsSenderDeviceId);

	_wifiManager.setConfigPortalTimeout(0);
	_wifiManager.setConfigPortalBlocking(false);

	Serial.printf("Start AP %s\r\n", WiFi.getHostname());

	// First parameter is name of access point, second is the password
	//_wifiManager.resetSettings();
	//*wm:StartAP with SSID:
	// auto res = _wifiManager.startConfigPortal(hostname, AP_PASSWORD);
	auto res = _wifiManager.autoConnect(hostname, AP_PASSWORD);
	if (!res)
	{
		Serial.println("WiFi : Failed to connect OR is running in non-blocking mode");
		// ESP.restart();
	}

	Serial.println("WebPortal setup complete");
}

////////////////////////////////////////////////////////////////////////////
/// @brief Setup all the URL bindings. Called when the server is ready
void WebPortal::OnBindServerCallback()
{
	Serial.println("Binding server callback");

	// Our main pages
	_wifiManager.server->on("/i", HTTP_GET, std::bind(&WebPortal::IndexHtml, this));
	_wifiManager.server->on("/index", HTTP_GET, std::bind(&WebPortal::IndexHtml, this));
	//_wifiManager.server->on("/castergraph", std::bind(&WebPortal::GraphHtml, this));
	_wifiManager.server->on("/status", HTTP_GET, std::bind(&WebPortal::ShowStatusHtml, this));
	_wifiManager.server->on("/log", HTTP_GET, [this]()
							{ HtmlLog("System log", _mainLog); });
	_wifiManager.server->on("/gpslog", HTTP_GET, [this]()
							{ HtmlLog("GPS log", _gpsParser.GetLogHistory()); });
}

////////////////////////////////////////////////////////////////////////////
/// @brief Process the look actions. This is called every loop only if the WiFi connection is available
void WebPortal::Loop()
{
	if (!_wifiManager.getConfigPortalActive())
	{
		// Process the WiFi manager (Restart if necessary)
		_wifiManager.startConfigPortal();
	}
	else
	{
		if (_loops++ > 10000)
		{
			_loops = 0;
			_wifiManager.process();
		}
	}
}

void WebPortal::OnSaveParamsCallback()
{
	Serial.println("SaveParamsCallback");
	_ntripClient.Save(_pNtripAddress->getValue(), _pNtripPort->getValue(), _pNtripMountPoint->getValue(), _pNtripUsername->getValue(), _pNtripPassword->getValue());
	_gpsParser.GetGpsSender().Save(_pGpsSenderUrl->getValue(), _pGpsSenderDeviceId->getValue());

	ESP.restart();
}

///////////////////////////////////////////////////////////////////////////////
/// @brief Display a log in html format
/// @param title Title of the log
/// @param log The log to display
void WebPortal::HtmlLog(const char *title, const std::vector<std::string> &log) const
{
	Serial.printf("Show %s\n", title);
	std::string html = "<h3>Log ";
	html += title;
	html += "</h3>";
	html += "<pre>";
	for (const auto &entry : log)
	{
		html += (Replace(ReplaceNewlineWithTab(entry), "<", "&lt;") + "\n");
	}
	html += "</pre>";
	_wifiManager.server->send(200, "text/html", html.c_str());
}

///////////////////////////////////////////////////////////////////////////////
/// @brief Create a string with n spaces
/// @param n Number of spaces
std::string I(int n)
{
	std::string repeatedString;
	for (int i = 0; i < n; ++i)
		repeatedString += "&nbsp; &nbsp; ";
	return repeatedString;
}

///////////////////////////////////////////////////////////////////////////////
/// @brief Add a table row to the html
/// @param html where to append the data
/// @param indent how many spaces to indent
/// @param name title of the row
/// @param value content of the row
/// @param rightAlign true if the value should be right aligned (Numbers)
void TableRow(std::string &html, int indent, const std::string &name, const char *value, bool rightAlign)
{
	html += "<tr>";
	switch (indent)
	{
	case 0:
		html += "<td class='i1'>";
		break;
	case 2:
		html += "<td class='i2'>";
		break;

	default:
		html += "<td>";
		break;
	}

	html += I(indent);
	html += name;
	html += "</td><td";
	if (rightAlign)
		html += " class='r'";
	html += ">";
	html += value;
	html += "</td></tr>";
}

void TableRow(std::string &html, int indent, const std::string &name, const char *value)
{
	TableRow(html, indent, name, value, false);
}
void TableRow(std::string &html, int indent, const std::string &name, const std::string &value)
{
	TableRow(html, indent, name, value.c_str());
}
void TableRow(std::string &html, int indent, const std::string &name, int32_t value)
{
	TableRow(html, indent, name, ToThousands(value).c_str(), true);
}

// void ServerStatsHtml(NTRIPServer &server, std::string &html)
// {
// 	TableRow(html, 2, "Address", server.GetAddress());
// 	TableRow(html, 3, "Port", server.GetPort());
// 	TableRow(html, 3, "Credential", server.GetCredential());
// 	TableRow(html, 3, "Status", server.GetStatus());
// 	TableRow(html, 3, "Reconnects", server.GetReconnects());
// 	TableRow(html, 3, "Packets sent", server.GetPacketsSent());
// 	TableRow(html, 3, "Speed (Mbps)", server.AverageSendTime());
// }

///////////////////////////////////////////////////////////////////////////////
/// @brief Display a list of possible pages
void WebPortal::IndexHtml()
{
	Serial.println("ShowIndexHtml");
	std::string html = "<head>\
	<link rel='stylesheet' href='https://cdn.jsdelivr.net/npm/@picocss/pico@2/css/pico.min.css'>\
	</head>\
	<body style='padding:10px;'>\
	<h3>Index</h3>";

	html += "<ul>";
	html += "<li><a href='/status'>System status</a></li>";
	html += "<li><a href='/info?'>Device info</a></li>";
	html += "<li><a href='/log'>System log</a></li>";
	html += "<li><a href='/gpslog'>GPS log</a></li>";
	html += "</ul>";
	html += "</body>";
	_wifiManager.server->send(200, "text/html", html.c_str());
}

void WebPortal::ShowStatusHtml()
{
	Serial.println("ShowStatusHtml");
	std::string html = "<head>\
	<link rel='stylesheet' href='https://cdn.jsdelivr.net/npm/@picocss/pico@2/css/pico.min.css'>\
	</head>\
	<body style='padding:10px;'>\
	<h3>System Status</h3>";
	html += "<style>\
	.r{text-align:right;}\
	.i1{color:red;}\
	.i2{color:blue;}\
	table{border-spacing: 0; width:fit-content;}\
	</style>";
	html += "<table class='striped'>";
	TableRow(html, 0, "General", "");
	TableRow(html, 1, "Version", APP_VERSION);
#if USER_SETUP_ID == 25
	TableRow(html, 1, "Device", "TTGO T-Display");
#else
#if USER_SETUP_ID == 206
	TableRow(html, 1, "Device", "TTGO T-Display-S3");
#else
	TableRow(html, 1, "Device", "Generic ESP32");
#endif
#endif
	// TableRow(html, 1, "Uptime", Uptime(millis()));
	TableRow(html, 1, "Free Heap", ESP.getFreeHeap());
	TableRow(html, 1, "Free Sketch Space", ESP.getFreeSketchSpace());
	TableRow(html, 0, "GPS", "");
	TableRow(html, 1, "Resets", _gpsParser.GetCommandQueue().GetResetCount());
	// TableRow(html, 1, "Device firmware", _gpsParser.GetCommandQueue().GetDeviceFirmware());
	// TableRow(html, 1, "Device serial #", _gpsParser.GetCommandQueue().GetDeviceSerial());

	// int32_t resetCount, reinitialize, packetCount;
	// _display.GetGpsStats(resetCount, reinitialize, packetCount);
	// TableRow(html, 1, "Reset count", resetCount);
	// TableRow(html, 1, "Reinitialize count", reinitialize);
	// TableRow(html, 1, "Packet count", packetCount);

	//	TableRow(html,"", );
	html += "</table></body>";
	_wifiManager.server->send(200, "text/html", html.c_str());
}