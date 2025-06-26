#pragma once

#include "Global.h"
#include "GpsParser.h"
#include "HandyString.h"
#include "History.h"
#include "NTRIPServer.h"
#include "WebPageWrapper.h"
#include "WebPageSettings.h"
#include <WiFiManager.h>
#include "WifiBusyTask.h"
#include "WiFiEvents.h"

extern WiFiManager _wifiManager;
extern NTRIPServer _ntripServer0;
extern NTRIPServer _ntripServer1;
extern NTRIPServer _ntripServer2;
extern GpsParser _gpsParser;
extern MyDisplay _display;
extern std::string _baseLocation;
extern History _history;

/// @brief Class manages the web pages displayed in the device.
class WebPortal
{
public:
	void Loop();

private:
	void OnBindServerCallback();
	void IndexHtml();
	void SettingsHtml();
	void ShowStatusHtml();
	void GraphHtml() const;
	void GraphDetail(WiFiClient &client, std::string divId, const NTRIPServer &server) const;
	void GraphTemperature() const;
	void GraphArray(WiFiClient &client, std::string divId, std::string title, const char *pBytes, int length) const;
	void HtmlLog(const char *title, const std::vector<std::string> &log) const;

	int _loops = 0;

public:
	/// @brief Startup the portal
	void Setup()
	{
		// Block here till we have WiFi credentials (good or bad)
		Logf("Start listening on %s", MakeHostName().c_str());
		WiFi.mode(WIFI_AP_STA);

		const int wifiTimeoutSeconds = 120;
		WifiBusyTask wifiBusy(_display);
		_wifiManager.setConfigPortalTimeout(wifiTimeoutSeconds);
		_wifiManager.setConfigPortalBlocking(true);
		while (WiFi.status() != WL_CONNECTED)
		{
			Logf("Try WIFI Connection on %s", MakeHostName().c_str());
			wifiBusy.StartCountDown(wifiTimeoutSeconds);
			_wifiManager.autoConnect(WiFi.getHostname(), AP_PASSWORD);
			// ESP.restart();
			// delay(1000);
		}

		// Setup callbacks
		_wifiManager.setWebServerCallback(std::bind(&WebPortal::OnBindServerCallback, this));
		_wifiManager.setConfigPortalTimeout(0);
		_wifiManager.setConfigPortalBlocking(false);

		// Make access point name
		std::string apName = "RtkSvr-";
		auto macAddress = WiFi.macAddress();
		macAddress.replace(":", "");
		apName += macAddress.c_str();

		// First parameter is name of access point, second is the password
		// Don't know why this is called again
		//_wifiManager.resetSettings();
		Logf("Start AP %s", apName.c_str());
		_wifiManager.autoConnect(apName.c_str(), AP_PASSWORD);

		//
		//_wifiManager.startWebPortal();
		_display.RefreshScreen();
		Logln("WebPortal setup complete");
	}
};

////////////////////////////////////////////////////////////////////////////
/// @brief Setup all the URL bindings. Called when the server is ready
void WebPortal::OnBindServerCallback()
{
	Logln("Binding server callback");

	// Our main pages
	_wifiManager.server->on(
		"/i", HTTP_GET, std::bind(&WebPortal::ShowStatusHtml, this));
	_wifiManager.server->on(
		"/log", HTTP_GET, [this]()
		{ HtmlLog("System log", CopyMainLog()); });
	_wifiManager.server->on("/gpslog", HTTP_GET,
							[this]()
							{ HtmlLog("GPS log", _gpsParser.GetLogHistory()); });
	_wifiManager.server->on("/caster1log", HTTP_GET,
							[this]()
							{ HtmlLog("Caster 1 log", _ntripServer0.GetLogHistory()); });
	_wifiManager.server->on("/caster2log", HTTP_GET,
							[this]()
							{ HtmlLog("Caster 2 log", _ntripServer1.GetLogHistory()); });
	_wifiManager.server->on("/caster3log", HTTP_GET,
							[this]()
							{ HtmlLog("Caster 3 log", _ntripServer2.GetLogHistory()); });

	_wifiManager.server->on(
		"/castergraph", std::bind(&WebPortal::GraphHtml, this));
	_wifiManager.server->on(
		"/tempGraph", std::bind(&WebPortal::GraphTemperature, this));

	_wifiManager.server->on(
		"/settings", HTTP_GET, std::bind(&WebPortal::SettingsHtml, this));

	_wifiManager.server->on("/RESET_WIFI", HTTP_GET,
							[this]()
							{
								Logln("Resetting WiFi settings");
								WiFiClient client = _wifiManager.server->client();
								auto p = WebPageWrapper(client);
								p.AddPageHeader(_wifiManager.server->uri().c_str());
								_wifiManager.erase();
								WebPageWrapper::RestartDevice(client, "https://github.com/mctainsh/Esp32/tree/main/UM98RTKServer#connect-wifi");
							});
	_wifiManager.server->on("/RESTART_ESP32", HTTP_GET,
							[this]()
							{
								Logln("Restarting");
								WiFiClient client = _wifiManager.server->client();
								auto p = WebPageWrapper(client);
								p.AddPageHeader(_wifiManager.server->uri().c_str());
								WebPageWrapper::RestartDevice(client, "/i");
							});
	_wifiManager.server->on("/FRESET_GPS_CONFIRMED", HTTP_GET,
							[this]()
							{
								Logln("Restarting GPS");
								_gpsParser.GetCommandQueue().IssueFReset();
								WiFiClient client = _wifiManager.server->client();
								auto p = WebPageWrapper(client);
								p.AddPageHeader(_wifiManager.server->uri().c_str());
								WebPageWrapper::RestartDevice(client, "/gpslog");
							});
}

////////////////////////////////////////////////////////////////////////////
/// @brief Process the look actions. This is called every loop only if the WiFi
/// connection is available
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

///////////////////////////////////////////////////////////////////////////////
/// @brief Plot a single graph
void WebPortal::GraphHtml() const
{
	WiFiClient client = _wifiManager.server->client();

	auto p = WebPageWrapper(client);
	p.AddPageHeader(_wifiManager.server->uri().c_str());

	client.print(
		"<script src='https://cdn.plot.ly/plotly-latest.min.js'></script>");
	client.print(
		"<h3>Average packet send time for the second (5 minutes total)</h3>");
	GraphDetail(client, "1", _ntripServer0);
	GraphDetail(client, "2", _ntripServer1);
	GraphDetail(client, "3", _ntripServer2);

	p.AddPageFooter();
}

///////////////////////////////////////////////////////////////////////////////
/// @brief Plot a single graph
/// @param html Where to append the graph
/// @param divId Id of the div to plot the graph in
/// @param server The server to plot. Source of title and data
void WebPortal::GraphDetail(
	WiFiClient &client, std::string divId, const NTRIPServer &server) const
{
	auto sendMicrosecondList = _history.GetNtripSendTime(server.GetIndex());
	std::string html = "<div id='myPlot" + divId + "' style='width:100%;max-width:700px'></div>\n";
	html += "<script>";
	client.print(html.c_str());

	html = "const xValues" + divId + " = [";
	for (int n = 0; n < sendMicrosecondList.size(); n++)
	{
		if (n != 0)
			html += ",";
		html += StringPrintf("%d", n);
	}
	html += "];";
	client.print(html.c_str());

	html = "const yValues" + divId + " = [";
	for (int n = 0; n < sendMicrosecondList.size(); n++)
	{
		if (n != 0)
			html += ",";
		html += StringPrintf("%d", sendMicrosecondList[n]);
	}
	html += "];";
	html += "Plotly.newPlot('myPlot" + divId + "', [{x:xValues" + divId +
			", y:yValues" + divId + ", mode:'lines'}], {title: '" +
			server.GetAddress() + " (&#181;s)'});";
	html += "</script>";
	client.print(html.c_str());
}

///////////////////////////////////////////////////////////////////////////////
/// @brief Plot a single graph
void WebPortal::GraphTemperature() const
{
	WiFiClient client = _wifiManager.server->client();
	auto p = WebPageWrapper(client);
	p.AddPageHeader(_wifiManager.server->uri().c_str());
	client.print("<script src='https://cdn.plot.ly/plotly-latest.min.js'></script>");
	client.print("<h3>24 hour Temperature</h3>");

	GraphArray(client, "T", "CPU Temperature (&deg;C)", _history.GetTemperatures(), TEMP_HISTORY_SIZE);

	p.AddPageFooter();
}

void WebPortal::GraphArray(WiFiClient &client, std::string divId, std::string title, const char *pBytes, int length) const
{
	client.print(std::string("<div id='myPlot" + divId + "' style='width:100%;max-width:700px'></div><script>").c_str());
	std::string html = "const xValues" + divId + " = [";
	for (int n = 0; n < length; n++)
	{
		if (n != 0)
			html += ",";
		html += StringPrintf("%d", n);
	}
	html += "];";
	client.print(html.c_str());

	html = "const yValues" + divId + " = [";
	for (int n = 0; n < length; n++)
	{
		if (n != 0)
			html += ",";
		html += StringPrintf("%d", pBytes[n]);
	}
	client.print(html.c_str());

	html = "]; Plotly.newPlot('myPlot" + divId + "', [{x:xValues" + divId +
		   ", y:yValues" + divId + ", mode:'lines'}], {title: '";
	client.print(html.c_str());
	client.print(title.c_str());
	client.print("'});</script>\n");
}

///////////////////////////////////////////////////////////////////////////////
/// @brief Display a log in html format
/// @param title Title of the log
/// @param log The log to display
/// TODO : Force to DOS Codepage 437
void WebPortal::HtmlLog(const char *title, const std::vector<std::string> &log) const
{
	Logf("Show '%s'", title);

	WiFiClient client = _wifiManager.server->client();
	auto p = WebPageWrapper(client);
	p.AddPageHeader(_wifiManager.server->uri().c_str());

	client.print("<h3>");
	client.print(title);
	client.println("</h3>");
	client.print("<pre>");
	for (const auto &entry : log)
	{
		client.print(Replace(ReplaceNewlineWithTab(entry), "<", "&lt;").c_str());
		client.print("\n");
	}
	client.println("</pre>");

	p.AddPageFooter();
}

///////////////////////////////////////////////////////////////////////////////
/// Show the main index page
void WebPortal::IndexHtml()
{
	//	<link rel='stylesheet' href='https://securehub.net/Esp32Rtk.css'>\
	Logln("ShowIndexHtml");

	WiFiClient client = _wifiManager.server->client();
	auto p = WebPageWrapper(client);
	p.AddPageHeader(_wifiManager.server->uri().c_str());
	p.AddPageFooter();
}

///////////////////////////////////////////////////////////////////////////////
/// @brief Show the setting page
void WebPortal::SettingsHtml()
{
	Logln("ShowSettingsHtml");

	WiFiClient client = _wifiManager.server->client();
	auto p = WebPageSettings(client);
	p.ShowHtml();
}

////////////////////////////////////////////////////////////////////////////////
void ServerStatsHtml(NTRIPServer &server, WebPageWrapper &p)
{
	p.GetClient().print("<td><Table class='table table-striped w-auto'>");
	p.TableRow(2, "Address", server.GetAddress());
	p.TableRow(3, "Port", server.GetPort());
	p.TableRow(3, "Credential", server.GetCredential());
	p.TableRow(3, "Status", server.GetStatus());
	p.TableRow(3, "Reconnects", server.GetReconnects());
	p.TableRow(3, "Packets sent", server.GetPacketsSent());
	p.TableRow(3, "Queue overflows", server.GetQueueOverflows());
	p.TableRow(3, "Send timeouts", server.GetTotalTimeouts());
	p.TableRow(3, "Expired packets", server.GetExpiredPackets());
	p.TableRow(
		3, "Median Send (&micro;s)", _history.MedianSendTime(server.GetIndex()));
	p.TableRow(3, "Max send (&#181;s)", server.GetMaxSendTime());
	p.TableRow(3, "Max Stack Height", server.GetMaxStackHeight());
	p.GetClient().print("</td></Table>");
}

///////////////////////////////////////////////////////////////////////////////
/// @brief Show the status of the system
void WebPortal::ShowStatusHtml()
{
	Logln("ShowStatusHtml");

	WiFiClient client = _wifiManager.server->client();
	auto p = WebPageWrapper(client);
	p.AddPageHeader(_wifiManager.server->uri().c_str());

	client.print("<h3>System Status</h3>");
	client.print("<style>\
	.r{text-align:right;}\
	.i1{color:red !important;}\
	.i2{color:blue !important;}\
	</style>");
	;
	client.print("<table class='table table-striped w-auto'>");
	p.TableRow(0, "General", "");
	p.TableRow(1, "Version", APP_VERSION);
#if USER_SETUP_ID == 25
	p.TableRow(1, "Device", "TTGO T-Display");
#else
#if USER_SETUP_ID == 206
	p.TableRow(1, "Device", "TTGO T-Display-S3");
#else
	p.TableRow(1, "Device", "Generic ESP32");
#endif
#endif
	p.TableRow(1, "Uptime", Uptime(millis()));
	p.TableRow(1, "Time", _handyTime.LongString());

	// Wifi stuff
	p.TableRow(0, "WI-FI", "");
	auto strength = WiFi.RSSI();
	std::string strengthTitle = "Unusable";
	if (WiFi.RSSI() > -30)
		strengthTitle = "Excellent";
	else if (WiFi.RSSI() > -67)
		strengthTitle = "Very Good";
	else if (WiFi.RSSI() > -70)
		strengthTitle = "Okay";
	else if (WiFi.RSSI() > -80)
		strengthTitle = "Not Good";

	p.TableRow(
		1, "WIFI Signal", std::to_string(strength) + "dBm " + strengthTitle);
	p.TableRow(1, "Mac Address", WiFi.macAddress().c_str());
	p.TableRow(1, "A/P name", WiFi.getHostname());
	p.TableRow(1, "IP Address", WiFi.localIP().toString().c_str());
	p.TableRow(1, "Host name", StringPrintf("%s.local", _mdnsHostName.c_str()).c_str());
	p.TableRow(1, "WiFi Mode",
			   WiFi.getMode() == WIFI_STA
				   ? "Station"
				   : (WiFi.getMode() == WIFI_AP
						  ? "Access Point"
						  : (WiFi.getMode() == WIFI_AP_STA ? "Access Point + Station"
														   : "Unknown")));

	// p.TableRow( 1, "Free Heap", ESP.getFreeHeap());
	p.TableRow(0, "GPS", "");
	p.TableRow(1, "Device type", _gpsParser.GetCommandQueue().GetDeviceType());
	p.TableRow(
		1, "Device firmware", _gpsParser.GetCommandQueue().GetDeviceFirmware());
	p.TableRow(
		1, "Device serial #", _gpsParser.GetCommandQueue().GetDeviceSerial());

	// Counters and stats
	int32_t resetCount, reinitialize, messageCount;
	_display.GetGpsStats(resetCount, reinitialize, messageCount);
	p.TableRow(1, "Reset count", resetCount);
	p.TableRow(1, "Reinitialize count", reinitialize);

	p.TableRow(1, "Bytes received", _gpsParser.GetGpsBytesRec());
	p.TableRow(1, "Reset count", _gpsParser.GetGpsResetCount());
	p.TableRow(1, "Reinitialize count", _gpsParser.GetGpsReinitialize());

	p.TableRow(1, "Read errors", _gpsParser.GetReadErrorCount());
	p.TableRow(1, "Max buffer size", _gpsParser.GetMaxBufferSize());

	p.TableRow(0, "Message counts", "");
	p.TableRow(1, "ASCII", _gpsParser.GetAsciiMsgCount());
	int totalRtkMessages = 0;
	for (const auto &pair : _gpsParser.GetMsgTypeTotals())
	{
		p.TableRow(1, std::to_string(pair.first), pair.second);
		totalRtkMessages += pair.second;
	}
	p.TableRow(1, "Total messages", totalRtkMessages);
	client.println("</table>");

	client.println("<Table><tr>");
	ServerStatsHtml(_ntripServer0, p);
	ServerStatsHtml(_ntripServer1, p);
	ServerStatsHtml(_ntripServer2, p);
	client.println("</tr></Table>");

	// Memory stuff
	client.println("<table class='table table-striped w-auto'>");
	auto free = ESP.getFreeHeap();
	auto total = ESP.getHeapSize();
	p.TableRow(0, "Memory", "");
	p.TableRow(1, "Stack High", uxTaskGetStackHighWaterMark(nullptr));
	p.TableRow(1, "Free Sketch Space", ESP.getFreeSketchSpace());
	p.TableRow(1, "Port free heap", xPortGetFreeHeapSize());
	p.TableRow(1, "Free heap", esp_get_free_heap_size());
	p.TableRow(1, "Heap", "");
	p.TableRow(2, "Free %", StringPrintf("%d%%", (int)(100.0 * free / total)));
	p.TableRow(2, "Free (Min)", ESP.getMinFreeHeap());
	p.TableRow(2, "Free (now)", free);
	p.TableRow(2, "Free (Max)", ESP.getMaxAllocHeap());
	p.TableRow(2, "Total", total);

	p.TableRow(1, "Total PSRAM", ESP.getPsramSize());
	p.TableRow(1, "Free PSRAM", ESP.getFreePsram());
#ifdef T_DISPLAY_S3
	p.TableRow(1, "spiram size", esp_spiram_get_size());

	p.TableRow(1, "Free", "");
	size_t dram_free = heap_caps_get_free_size(MALLOC_CAP_DMA);
	p.TableRow(2, "DRAM (MALLOC_CAP_DMA)", dram_free);

	size_t internal_ram_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
	p.TableRow(2, "Internal RAM (MALLOC_CAP_INTERNAL)", internal_ram_free);

	p.TableRow(2, "SPIRAM (MALLOC_CAP_SPIRAM)",
			   heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
	p.TableRow(2, "Default Memory (MALLOC_CAP_DEFAULT)",
			   heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
	p.TableRow(
		2, "IRAM (MALLOC_CAP_EXEC)", heap_caps_get_free_size(MALLOC_CAP_EXEC));
#endif

	// p.TableRow( 1, "himem free", esp_himem_get_free_size());
	// p.TableRow( 1, "himem phys", esp_himem_get_phys_size());
	// p.TableRow( 1, "himem reserved", esp_himem_reserved_area_size());

	client.println("</table>");
	p.AddPageFooter();
}
