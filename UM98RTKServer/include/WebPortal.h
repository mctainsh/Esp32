#pragma once

#include "Global.h"
#include <WiFiManager.h>
#include "HandyString.h"
#include "NTRIPServer.h"
#include "GpsParser.h"
#include "History.h"
#include "WebPageWrapper.h"

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
	void Setup();
	void Loop();

private:
	void OnBindServerCallback();
	void IndexHtml();
	void ConfirmResetHtml();
	void ShowStatusHtml();
	void GraphHtml() const;
	void GraphDetail(WiFiClient& client, std::string divId, const NTRIPServer &server) const;
	void GraphTemperature() const;
	void GraphArray(WiFiClient &client, std::string divId, std::string title, const char *pBytes, int length) const;
	void HtmlLog(const char *title, const std::vector<std::string> &log) const;
	void OnSaveParamsCallback();

	int _loops = 0;

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

	WiFiManagerParameter *_pBaseLocation;
};

/// @brief Startup the portal
void WebPortal::Setup()
{
	// Setup callbacks
	_wifiManager.setWebServerCallback(std::bind(&WebPortal::OnBindServerCallback, this));
	_wifiManager.setSaveParamsCallback([this]()
									   { OnSaveParamsCallback(); });

	std::string port0String = std::to_string(_ntripServer0.GetPort());
	_pCaster0Address = new WiFiManagerParameter("address0", "Caster 1 address", _ntripServer0.GetAddress().c_str(), 40);
	_pCaster0Port = new WiFiManagerParameter("port0", "Caster 1 port [Normally 2101] (0 = off)", port0String.c_str(), 6);
	_pCaster0Credential = new WiFiManagerParameter("credential0", "Caster 1 credential ", _ntripServer0.GetCredential().c_str(), 40);
	_pCaster0Password = new WiFiManagerParameter("password0", "Caster 1 password", _ntripServer0.GetPassword().c_str(), 40);

	std::string port1String = std::to_string(_ntripServer1.GetPort());
	_pCaster1Address = new WiFiManagerParameter("address1", "Caster 2 address", _ntripServer1.GetAddress().c_str(), 40);
	_pCaster1Port = new WiFiManagerParameter("port1", "Caster 2 port (0 = off)", port1String.c_str(), 6);
	_pCaster1Credential = new WiFiManagerParameter("credential1", "Caster 2 credential", _ntripServer1.GetCredential().c_str(), 40);
	_pCaster1Password = new WiFiManagerParameter("password1", "Caster 2 password", _ntripServer1.GetPassword().c_str(), 40);

	std::string port2String = std::to_string(_ntripServer2.GetPort());
	_pCaster2Address = new WiFiManagerParameter("address2", "Caster 3 address", _ntripServer2.GetAddress().c_str(), 40);
	_pCaster2Port = new WiFiManagerParameter("port2", "Caster 3 port (0 = off)", port2String.c_str(), 6);
	_pCaster2Credential = new WiFiManagerParameter("credential2", "Caster 3 credential", _ntripServer2.GetCredential().c_str(), 40);
	_pCaster2Password = new WiFiManagerParameter("password2", "Caster 3 password", _ntripServer2.GetPassword().c_str(), 40);

	_pBaseLocation = new WiFiManagerParameter("baseLocation", "Base Location (Lat Long Height)", _baseLocation.c_str(), 100);

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

	_wifiManager.addParameter(_pBaseLocation);

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
	//Logf("Start AP %s", apName.c_str());
	//auto res = _wifiManager.autoConnect(apName.c_str(), AP_PASSWORD);
	//if (!res)
	//	Logln("WiFi : Failed to connect OR is running in non-blocking mode");

	//
	//_wifiManager.startWebPortal();

	Logln("WebPortal setup complete");
}

////////////////////////////////////////////////////////////////////////////
/// @brief Setup all the URL bindings. Called when the server is ready
void WebPortal::OnBindServerCallback()
{
	Logln("Binding server callback");

	// Our main pages
	_wifiManager.server->on("/Confirm_Reset", HTTP_GET, std::bind(&WebPortal::ConfirmResetHtml, this));
	_wifiManager.server->on("/i", HTTP_GET, std::bind(&WebPortal::ShowStatusHtml, this));
	_wifiManager.server->on("/log", HTTP_GET, [this]()
							{ HtmlLog("System log", CopyMainLog()); });
	_wifiManager.server->on("/gpslog", HTTP_GET, [this]()
							{ HtmlLog("GPS log", _gpsParser.GetLogHistory()); });
	_wifiManager.server->on("/caster1log", HTTP_GET, [this]()
							{ HtmlLog("Caster 1 log", _ntripServer0.GetLogHistory()); });
	_wifiManager.server->on("/caster2log", HTTP_GET, [this]()
							{ HtmlLog("Caster 2 log", _ntripServer1.GetLogHistory()); });
	_wifiManager.server->on("/caster3log", HTTP_GET, [this]()
							{ HtmlLog("Caster 3 log", _ntripServer2.GetLogHistory()); });

	_wifiManager.server->on("/castergraph", std::bind(&WebPortal::GraphHtml, this));
	_wifiManager.server->on("/tempGraph", std::bind(&WebPortal::GraphTemperature, this));

	_wifiManager.server->on("/FRESET_GPS_CONFIRMED", HTTP_GET, [this]()
							{ 
								_gpsParser.GetCommandQueue().IssueFReset();
								_wifiManager.server->send(200, "text/html", "<html>Done</html>"); });
	_wifiManager.server->on("/RESET_WIFI", HTTP_GET, [this]()
							{ 
								_wifiManager.erase();
								ESP.restart(); });
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
	Logf("SaveParamsCallback");

	_ntripServer0.Save(_pCaster0Address->getValue(), _pCaster0Port->getValue(), _pCaster0Credential->getValue(), _pCaster0Password->getValue());
	_ntripServer1.Save(_pCaster1Address->getValue(), _pCaster1Port->getValue(), _pCaster1Credential->getValue(), _pCaster1Password->getValue());
	_ntripServer2.Save(_pCaster2Address->getValue(), _pCaster2Port->getValue(), _pCaster2Credential->getValue(), _pCaster2Password->getValue());

	SaveBaseLocation(_pBaseLocation->getValue());

	delay(1000);

	ESP.restart();
}

///////////////////////////////////////////////////////////////////////////////
/// @brief Plot a single graph
void WebPortal::GraphHtml() const
{
		WiFiClient client = _wifiManager.server->client();

	auto p = WebPageWrapper(client);
	p.AddPageHeader(_wifiManager.server->uri().c_str());


	client.print("<script src='https://cdn.plot.ly/plotly-latest.min.js'></script>");
	client.print("<h3>Graphs of send time per packet</h3>");
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
void WebPortal::GraphDetail(WiFiClient& client, std::string divId, const NTRIPServer &server) const
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
	html += "Plotly.newPlot('myPlot" + divId + "', [{x:xValues" + divId + ", y:yValues" + divId + ", mode:'lines'}], {title: '" + server.GetAddress() + " (&#181;s)'});";
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

void WebPortal::GraphArray(WiFiClient& client, std::string divId, std::string title, const char *pBytes, int length) const
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

	html = "]; Plotly.newPlot('myPlot" + divId + "', [{x:xValues" + divId + ", y:yValues" + divId + ", mode:'lines'}], {title: '";
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
	Logf("Show %s", title);

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
/// @brief Confirm the GPS Reset
void WebPortal::ConfirmResetHtml()
{
	Logln("ShowConfirmReset");

	WiFiClient client = _wifiManager.server->client();
	auto p = WebPageWrapper(client);
	p.AddPageHeader(_wifiManager.server->uri().c_str());

	client.println(R"rawliteral(
		<div class="container py-4">
			<h3 class="mb-4 text-center">Confirm Reset</h3>
			<div class="d-grid gap-3 col-6 mx-auto">
			<a href="/FRESET_GPS_CONFIRMED" class="btn btn-warning btn-lg">Confirm GPS Reset</a>
			<a href="/RESET_WIFI" class="btn btn-danger btn-lg">Confirm Wi-Fi & Settings Reset</a>
			<a href="/i" class="btn btn-secondary btn-lg">Cancel</a>
			</div>
		</div>)rawliteral");

	p.AddPageFooter();
}

////////////////////////////////////////////////////////////////////////////////
void ServerStatsHtml(NTRIPServer &server, WebPageWrapper &p)
{
	p.GetClient().print("<td><Table class='table table-striped w-auto'>");
	p.TableRow( 2, "Address", server.GetAddress());
	p.TableRow( 3, "Port", server.GetPort());
	p.TableRow( 3, "Credential", server.GetCredential());
	p.TableRow( 3, "Status", server.GetStatus());
	p.TableRow( 3, "Reconnects", server.GetReconnects());
	p.TableRow( 3, "Packets sent", server.GetPacketsSent());
	p.TableRow( 3, "Queue overflows", server.GetQueueOverflows());
	p.TableRow( 3, "Send timeouts", server.GetTotalTimeouts());
	p.TableRow( 3, "Expired packets", server.GetExpiredPackets());
	p.TableRow( 3, "Median Send (&micro;s)", _history.MedianSendTime(server.GetIndex()));
	p.TableRow( 3, "Max send (&#181;s)", server.GetMaxSendTime());
	p.TableRow( 3, "Max Stack Height", server.GetMaxStackHeight());
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
	</style>");;
	client.print("<table class='table table-striped w-auto'>");
	p.TableRow( 0, "General", "");
	p.TableRow( 1, "Version", APP_VERSION);
#if USER_SETUP_ID == 25
	p.TableRow( 1, "Device", "TTGO T-Display");
#else
#if USER_SETUP_ID == 206
	p.TableRow( 1, "Device", "TTGO T-Display-S3");
#else
	p.TableRow( 1, "Device", "Generic ESP32");
#endif
#endif
	p.TableRow( 1, "Uptime", Uptime(millis()));
	p.TableRow( 1, "Time", _handyTime.LongString());

	// Wifi stuff
	p.TableRow( 0, "WI-FI", "");
	auto strength = WiFi.RSSI();
	std::string strengthTitle = "Unusable";
	if( WiFi.RSSI() > -30 )
		strengthTitle = "Excellent";
	else if( WiFi.RSSI() > -67 )
		strengthTitle = "Very Good";
	else if( WiFi.RSSI() > -70 )
		strengthTitle = "Okay";
	else if( WiFi.RSSI() > -80 )
		strengthTitle = "Not Good";

	p.TableRow( 1, "WIFI Signal", std::to_string(strength) + "dBm " + strengthTitle);
	p.TableRow( 1, "Mac Address", WiFi.macAddress().c_str());
	p.TableRow( 1, "A/P name", WiFi.getHostname());
	p.TableRow( 1, "IP Address", WiFi.localIP().toString().c_str());
	p.TableRow( 1, "WiFi Mode", WiFi.getMode() == WIFI_STA ? "Station" :
		(WiFi.getMode() == WIFI_AP ? "Access Point" :
		 (WiFi.getMode() == WIFI_AP_STA ? "Access Point + Station" : "Unknown")));

	// p.TableRow( 1, "Free Heap", ESP.getFreeHeap());
	p.TableRow( 0, "GPS", "");
	p.TableRow( 1, "Device type", _gpsParser.GetCommandQueue().GetDeviceType());
	p.TableRow( 1, "Device firmware", _gpsParser.GetCommandQueue().GetDeviceFirmware());
	p.TableRow( 1, "Device serial #", _gpsParser.GetCommandQueue().GetDeviceSerial());

	// Counters and stats
	int32_t resetCount, reinitialize, messageCount;
	_display.GetGpsStats(resetCount, reinitialize, messageCount);
	p.TableRow( 1, "Reset count", resetCount);
	p.TableRow( 1, "Reinitialize count", reinitialize);

	p.TableRow( 1, "Bytes received", _gpsParser.GetGpsBytesRec());
	p.TableRow( 1, "Reset count", _gpsParser.GetGpsResetCount());
	p.TableRow( 1, "Reinitialize count", _gpsParser.GetGpsReinitialize());

	p.TableRow( 1, "Read errors", _gpsParser.GetReadErrorCount());
	p.TableRow( 1, "Max buffer size", _gpsParser.GetMaxBufferSize());

	p.TableRow( 0, "Message counts", "");
	p.TableRow( 1, "ASCII", _gpsParser.GetAsciiMsgCount());
	int totalRtkMessages = 0;
	for (const auto &pair : _gpsParser.GetMsgTypeTotals())
	{
		p.TableRow( 1, std::to_string(pair.first), pair.second);
		totalRtkMessages += pair.second;
	}
	p.TableRow( 1, "Total messages", totalRtkMessages);
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
	p.TableRow( 0, "Memory", "");
	p.TableRow( 1, "Stack High", uxTaskGetStackHighWaterMark(nullptr));
	p.TableRow( 1, "Free Sketch Space", ESP.getFreeSketchSpace());
	p.TableRow( 1, "Port free heap", xPortGetFreeHeapSize());
	p.TableRow( 1, "Free heap", esp_get_free_heap_size());
	p.TableRow( 1, "Heap", "");
	p.TableRow( 2, "Free %", StringPrintf("%d%%", (int)(100.0 * free / total)));
	p.TableRow( 2, "Free (Min)", ESP.getMinFreeHeap());
	p.TableRow( 2, "Free (now)", free);
	p.TableRow( 2, "Free (Max)", ESP.getMaxAllocHeap());
	p.TableRow( 2, "Total", total);

	p.TableRow( 1, "Total PSRAM", ESP.getPsramSize());
	p.TableRow( 1, "Free PSRAM", ESP.getFreePsram());
#ifdef T_DISPLAY_S3
	p.TableRow( 1, "spiram size", esp_spiram_get_size());

	p.TableRow( 1, "Free", "");
	size_t dram_free = heap_caps_get_free_size(MALLOC_CAP_DMA);
	p.TableRow( 2, "DRAM (MALLOC_CAP_DMA)", dram_free);

	size_t internal_ram_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
	p.TableRow( 2, "Internal RAM (MALLOC_CAP_INTERNAL)", internal_ram_free);

	p.TableRow( 2, "SPIRAM (MALLOC_CAP_SPIRAM)", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
	p.TableRow( 2, "Default Memory (MALLOC_CAP_DEFAULT)", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
	p.TableRow( 2, "IRAM (MALLOC_CAP_EXEC)", heap_caps_get_free_size(MALLOC_CAP_EXEC));
#endif

	// p.TableRow( 1, "himem free", esp_himem_get_free_size());
	// p.TableRow( 1, "himem phys", esp_himem_get_phys_size());
	// p.TableRow( 1, "himem reserved", esp_himem_reserved_area_size());

	client.println("</table>");
	p.AddPageFooter();
}
