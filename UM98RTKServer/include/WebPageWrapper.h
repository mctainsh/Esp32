#pragma once
#include <WiFi.h>

///////////////////////////////////////////////////////////////////////////////
// Fancy HTML pages for the web portal
class WebPageWrapper
{
private:
	WiFiClient &_client;

public:
	WebPageWrapper(WiFiClient &c) : _client(c)
	{
	}

	///////////////////////////////////////////////////////////////////////////////
	/// @brief Display a list of possible pages
	void AddPageHeader(const char *currentUrl)
	{
		_client.println("<html><head>");
		// pClient.println("<link rel='stylesheet' href='https://cdn.jsdelivr.net/npm/@picocss/pico@2/css/pico.min.css'>");
		_client.println("<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.7/dist/css/bootstrap.min.css' rel='stylesheet' integrity='LN+7fdVzj6u52u30Kp6M/trliBMCMKTyK833zpbD+pXdCLuTusPj697FH4R/5mcr' crossorigin='anonymous'>");
		_client.println("<script src='https://cdn.jsdelivr.net/npm/bootstrap@5.3.7/dist/js/bootstrap.bundle.min.js' integrity='ndDqU0Gzau9qJ1lfW4pNLlhNTkCfHzAVBReH9diLvGRem5+R9g2FzA8ZGN954O5Q' crossorigin='anonymous'></script>");
		_client.println("</head>");
		_client.println("<body style='padding:10px;'>");
		_client.println("<main>");
		_client.println("<ul class='nav nav-pills nav-fill gap-2 p-1 small bg-primary rounded-5 shadow-sm' id='pillNav2' role='tablist' ");
		_client.println("style='--bs-nav-link-color: var(--bs-white); --bs-nav-pills-link-active-color: var(--bs-primary); --bs-nav-pills-link-active-bg: var(--bs-white);'>");

		//<nav class='menu' id='nav'>";
		AddMenuBarItem(currentUrl, "Status", "/status");
		AddMenuBarItem(currentUrl, "Device Info", "/info?");
		AddMenuBarItem(currentUrl, "Log", "/log");
		AddMenuBarItem(currentUrl, "GPS", "/gpslog");
		AddMenuBarItem(currentUrl, "Caster 1", "/caster1log");
		AddMenuBarItem(currentUrl, "Caster 2", "/caster2log");
		AddMenuBarItem(currentUrl, "Caster 3", "/caster3log");
		AddMenuBarItem(currentUrl, "Caster Graph", "/castergraph");
		AddMenuBarItem(currentUrl, "Caster Graph", "/castergraph");
		AddMenuBarItem(currentUrl, "Temperature Graph", "/tempGraph");
		AddMenuBarItem(currentUrl, "Reset", "/Confirm_Reset");
		_client.println("</ul>");
	}

	void AddPageFooter()
	{
		_client.println("</main>");
		_client.println("</body></html>");
	}

	void AddMenuBarItem(const char *currentUrl, const std::string &name, const std::string &url)
	{
		_client.println("<li class='nav-item' role='presentation'>");

		_client.println("<button class='nav-link ");
		if (currentUrl == url)
			_client.println(" active ");
		_client.println(" rounded-5' data-bs-toggle='tab' ");
		_client.printf("	onclick=\"window.location.href='%s';\" ", url.c_str());
		_client.printf("	type='button' role='tab' aria-selected='false'>%s</button>", name.c_str());

		_client.println("</li>");
	}
};
