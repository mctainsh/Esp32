#pragma once
#include <WiFi.h>

///////////////////////////////////////////////////////////////////////////////
// Fancy HTML pages for the web portal
class WebPageWrapper
{
private:
	WiFiClient &_client;

public:
	WiFiClient &GetClient() { return _client; }

	WebPageWrapper(WiFiClient &c) : _client(c)
	{
	}

	void Print(const std::string &text)
	{
		_client.print(text.c_str());
	}

	///////////////////////////////////////////////////////////////////////////////
	/// @brief Display a list of possible pages
	void AddPageHeader(const char *currentUrl)
	{
		_client.println("<html><head>");
		//_client.println("<link rel='stylesheet' href='https://cdn.jsdelivr.net/npm/@picocss/pico@2/css/pico.min.css'>");
		_client.println("<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.7/dist/css/bootstrap.min.css' rel='stylesheet' integrity='LN+7fdVzj6u52u30Kp6M/trliBMCMKTyK833zpbD+pXdCLuTusPj697FH4R/5mcr' crossorigin='anonymous'>");
		_client.println("<script src='https://cdn.jsdelivr.net/npm/bootstrap@5.3.7/dist/js/bootstrap.bundle.min.js' integrity='ndDqU0Gzau9qJ1lfW4pNLlhNTkCfHzAVBReH9diLvGRem5+R9g2FzA8ZGN954O5Q' crossorigin='anonymous'></script>");
		_client.println("</head>");
		_client.println("<body><main>");
		_client.println("<div style='position: fixed; top: 0;left: 0;right: 0;height: 60px;background-color: #000A;display: flex;align-items: center;justify-content: center;z-index: 1000;'>");

		_client.println("<ul class='nav nav-pills nav-fill gap-2 p-1 small bg-primary rounded-5 shadow-sm' id='pillNav2' role='tablist' ");
		_client.println("style='--bs-nav-link-color: var(--bs-white); --bs-nav-pills-link-active-color: var(--bs-primary); --bs-nav-pills-link-active-bg: var(--bs-white);'>");

		//<nav class='menu' id='nav'>";
		AddMenuBarItem(currentUrl, "Status", "/i");
		AddMenuBarItem(currentUrl, "Device Info", "/info?");
		AddMenuBarItem(currentUrl, "Log", "/log");
		AddMenuBarItem(currentUrl, "GPS", "/gpslog");
		AddMenuBarItem(currentUrl, "Caster 1", "/caster1log");
		AddMenuBarItem(currentUrl, "Caster 2", "/caster2log");
		AddMenuBarItem(currentUrl, "Caster 3", "/caster3log");
		AddMenuBarItem(currentUrl, "Caster Graph", "/castergraph");
		AddMenuBarItem(currentUrl, "Temperature Graph", "/tempGraph");
		AddMenuBarItem(currentUrl, "Reset", "/Confirm_Reset");
		_client.println("</ul></div>");

		// Body of the page
		_client.println("<div style='margin-top: 60px; padding: 20px;'>");

	}

	void AddPageFooter()
	{
		_client.println("</div></main></body></html>");
		_client.flush();
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

	//////////////////////////////////////////////////////////////////////////////
	/// @brief Add a table row to the html
	/// @param html where to append the data
	/// @param indent how many spaces to indent
	/// @param name title of the row
	/// @param value content of the row
	/// @param rightAlign true if the value should be right aligned (Numbers)
	void TableRow(int indent, const std::string &name, const char *value, bool rightAlign)
	{
		Serial.printf("TR : %s -> %s\r\n", name.c_str(), value);
		_client.print("<tr>");
		switch (indent)
		{
		case 0:
			_client.print("<td class='i1'>");
			break;
		case 2:
			_client.print("<td class='i2'>");
			break;

		default:
			_client.print("<td>");
			break;
		}

		_client.print(I(indent).c_str());
		_client.print(name.c_str());
		_client.print("</td><td");
		if (rightAlign)
			_client.print(" class='r'");
		_client.print(">");
		_client.print(value);
		_client.println("</td></tr>");
	}

	void TableRow(int indent, const std::string &name, const char *value)
	{
		TableRow(indent, name, value, false);
	}
	void TableRow(int indent, const std::string &name, const std::string &value)
	{
		TableRow(indent, name, value.c_str());
	}
	void TableRow(int indent, const std::string &name, int32_t value)
	{
		TableRow(indent, name, ToThousands(value).c_str(), true);
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
};
