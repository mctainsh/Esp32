#pragma once
#include <WiFi.h>

extern WiFiManager _wifiManager;
extern NTRIPServer _ntripServer0;
extern NTRIPServer _ntripServer1;
extern NTRIPServer _ntripServer2;

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
	//<script src="https://ajax.googleapis.com/ajax/libs/jquery/3.6.0/jquery.min.js"></script>
	void AddPageHeader(const char *currentUrl)
	{
		_client.println(R"rawliteral(
			<!DOCTYPE html><html><head>
			<link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.7/dist/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-LN+7fdVzj6u52u30Kp6M/trliBMCMKTyK833zpbD+pXdCLuTusPj697FH4R/5mcr" crossorigin="anonymous">
			<script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.7/dist/js/bootstrap.bundle.min.js" integrity="sha384-ndDqU0Gzau9qJ1lfW4pNLlhNTkCfHzAVBReH9diLvGRem5+R9g2FzA8ZGN954O5Q" crossorigin="anonymous"></script>
			<link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap-icons@1.13.1/font/bootstrap-icons.css">
			<script>
			document.addEventListener("DOMContentLoaded", function () {
				const popoverTriggerList = document.querySelectorAll('[data-bs-toggle="popover"]');
				popoverTriggerList.forEach(el => new bootstrap.Popover(el));
			});
			</script>
			</head>
			<body><main>
			<div style='position: fixed; top: 0;left: 0;right: 0;height: 60px;background-color: #000A;display: flex;align-items: center;justify-content: center;z-index: 1000;'>

			<ul class='nav nav-pills nav-fill gap-2 p-1 small bg-primary rounded-5 shadow-sm' id='pillNav2' role='tablist'
				style='--bs-nav-link-color: var(--bs-white); --bs-nav-pills-link-active-color: var(--bs-primary); --bs-nav-pills-link-active-bg: var(--bs-white);'>
		)rawliteral");

		//<nav class='menu' id='nav'>";
		AddMenuBarItem(currentUrl, "Status", "/i");
		// AddMenuBarItem(currentUrl, "Device Info", "/info?");
		AddMenuBarItem(currentUrl, "Log", "/log");
		AddMenuBarItem(currentUrl, "GPS", "/gpslog");
		if (_ntripServer0.IsEnabled())
			AddMenuBarItem(currentUrl, "Caster 1", "/caster1log");
		if (_ntripServer1.IsEnabled())
			AddMenuBarItem(currentUrl, "Caster 2", "/caster2log");
		if (_ntripServer2.IsEnabled())
			AddMenuBarItem(currentUrl, "Caster 3", "/caster3log");
		AddMenuBarItem(currentUrl, "Caster Graph", "/castergraph");
		AddMenuBarItem(currentUrl, "Temperature", "/tempGraph");
		AddMenuBarItem(currentUrl, "Settings", "/settings");
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
	/// @brief Add a help button with a popover
	std::string MakeHelpButton(const std::string &title, const std::string &content)
	{
		std::string html = "<button type='button' class='btn btn-outline-primary' style='padding: 1px 5px 1px 5px;' ";
		html += "data-bs-toggle='popover' title='" + title + "' ";
		html += "data-bs-content='" + content + "'>";
		html += "<i class='bi bi-question-circle'></i>";
		html += "</button>";
		return html;
	}

	std::string AddButtonWithEnable(const std::string &name, const std::string &title, const std::string &url)
	{
		_client.print("<div class='d-flex align-items-center gap-3'>");
		_client.printf("<a id='%sLink' href='%s' class='btn btn-warning btn-lg disabled' tabindex='-1' aria-disabled='true'>", name.c_str(), url.c_str());
		_client.print(title.c_str());
		_client.printf("</a><div class='form-check form-switch'>\
    			<input class='form-check-input' type='checkbox' id='%sSwitch'>\
    			<label class='form-check-label' for='%sSwitch'>Enable</label>\
  			</div></div>",
					   name.c_str(), name.c_str());

		_client.printf("\n<script>document.getElementById('%sSwitch').addEventListener('change', function () {const link = document.getElementById('%sLink');",
					   name.c_str(), name.c_str());
		_client.println("if (this.checked) {\
				link.classList.remove('disabled');\
				link.removeAttribute('aria-disabled');\
				link.removeAttribute('tabindex');\
				} else {\
				link.classList.add('disabled');\
				link.setAttribute('aria-disabled', 'true');\
				link.setAttribute('tabindex', '-1');\
				}\
				});\
				</script>)");
		return name;
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
