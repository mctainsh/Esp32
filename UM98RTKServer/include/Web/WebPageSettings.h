#pragma once

#include "Web\WebPageWrapper.h"
#include "NTRIPServer.h"
#include "HandyLog.h"
#include "HandyString.h"
#include "GpsParser.h"
#include "MyFiles.h"

extern NTRIPServer _ntripServer0;
extern NTRIPServer _ntripServer1;
extern NTRIPServer _ntripServer2;
extern std::string _baseLocation;
extern GpsParser _gpsParser;
extern MyFiles _myFiles;

///////////////////////////////////////////////////////////////////////////////
// Fancy HTML pages for the web portal
class WebPageSettings : public WebPageWrapper
{
private:
public:
	WebPageSettings(WiFiClient &c) : WebPageWrapper(c)
	{
	}

	///////////////////////////////////////////////////////////////////////////////
	/// @brief Confirm the GPS Reset
	void ShowHtml()
	{
		Logln("ShowSettingsHtml");

		AddPageHeader(_wifiManager.server->uri().c_str());

		_client.println("<style>.flex-row { display: flex;flex-wrap: wrap;gap: 1rem;}.flex-item "
						"{flex: 1 1 300px; min-width: 300px;background-color: #0001;box-sizing: border-box;}</style>");

		// Add the form for the caster 1
		_client.print("<h3 class='mt-4'>NTRIP Caster Settings</h3>"
					  "<div class='flex-row'>");
		AddCasterForm(_ntripServer0);
		AddCasterForm(_ntripServer1);
		AddCasterForm(_ntripServer2);
		_client.println("</div>");

		// Add Station location form
		AddStationLocationForm();

		// Add the form to set the station location
		AddMultiCastDNSForm();

		// Add Timezone offset
		AddTimezoneOffset();

		// Reset section
		_client.println(R"rawliteral(
<div class="accordion accordion-flush card" id="acd2">
	<div class="accordion-item">
		<h2 class="accordion-header">
			<button class="accordion-button collapsed" type="button" data-bs-toggle="collapse" data-bs-target="#flush-collapseOne" aria-expanded="false" aria-controls="flush-collapseOne">
			Reboot the device			
			</button>
		</h2>
		<div id="flush-collapseOne" class="accordion-collapse collapse" data-bs-parent="#accordionFlushExample">
		<div class="accordion-body">
			<strong>This will reboot the ESP32 but will not restart the GPS.</strong>
			<p>Are you sure you want to reboot the device?</p>
			<a href="/RESTART_ESP32" class="btn btn-warning btn-lg">Confirm reboot</a>	  
		</div>
		</div>
	</div>

  <div class="accordion-item">
    <h2 class="accordion-header">
      <button class="accordion-button collapsed" type="button" data-bs-toggle="collapse" data-bs-target="#flush-collapseTwo" aria-expanded="false" aria-controls="flush-collapseTwo">
  		Reset GPS settings
      </button>
    </h2>
    <div id="flush-collapseTwo" class="accordion-collapse collapse" data-bs-parent="#accordionFlushExample">
      <div class="accordion-body">
        <strong>This erase all GPS settings.</strong>
		<p>Are you sure erase GPS settings. This will force a average location of GPS to be also lost. Are you sure?</p>
		<a href="/FRESET_GPS_CONFIRMED" class="btn btn-warning btn-lg">Confirm GPS Reset</a>	 
	  </div>
    </div>
  </div>

  <div class="accordion-item">
    <h2 class="accordion-header">
      <button class="accordion-button collapsed" type="button" data-bs-toggle="collapse" data-bs-target="#flush-collapseThree" aria-expanded="false" aria-controls="flush-collapseThree">
		Reset Wi-Fi settings
      </button>
    </h2>
    <div id="flush-collapseThree" class="accordion-collapse collapse" data-bs-parent="#accordionFlushExample">
      <div class="accordion-body">
        <strong>This will erase all Wi-Fi setting. You will need to re enter the Wi-Fi credentials</strong> <p>Once reset, follow the Wi-Fi connection instructions to connect to the device again.</p>
		<p>Are you sure you want to reset the Wi-Fi settings?</p>
		<a href="/RESET_WIFI" class="btn btn-danger btn-lg">Confirm Wi-Fi & Settings Reset</a>
    </div>
  </div>
</div>
		)rawliteral");

		AddPageFooter();
	}

	///////////////////////////////////////////////////////////////////////////////
	/// @brief Add the form to set the station location
	void AddStationLocationForm()
	{
		// Title and help button
		_client.printf("<h3 class='mt-4'>Station calibrated location %s</h3>",
					   MakeHelpButton("Help",
									  "The precise location of the base station in the format (ie -27.57012345 153.09912345 35.258). "
									  "Leave blank for the station to auto calibrate. If provided, tt is critical this is accurate.")
						   .c_str());

		// Base location setup
		const char *BASE_LCN = "baseLocation"; // Actual location
		if (_wifiManager.server->hasArg(BASE_LCN))
		{
			SaveBaseLocation(_wifiManager.server->arg(BASE_LCN).c_str());
			_gpsParser.GetCommandQueue().IssueFReset();
			_client.println("<div class='alert alert-success' role='alert'>Base station location updated successfully!</div>");
		}

		// Main input form for the base location with update button
		_client.printf(R"rawliteral(
			<form method='get' class='container py-4 m-0 p-0'>
			<div class="input-group mb-3">
				<div class="form-floating">
					<input type="text" class="form-control" name="%s" id="%s" value="%s">
					<label for="%s" class="form-label">Latitude(&deg;) Longitude(&deg;) Height(m)</label>
				</div>
				<button class="btn btn-primary" type='submit' id="button-addon2">Apply</button>
			</div></form>	)rawliteral",
					   BASE_LCN, BASE_LCN, _baseLocation.c_str(), BASE_LCN);
	}

	///////////////////////////////////////////////////////////////////////////////
	/// @brief Add the form to set DNS for the ESP32
	void AddMultiCastDNSForm()
	{
		// Title and help button
		_client.printf("<h3 class='mt-4'>Multicast DNS (Bonjour name) %s</h3>",
					   MakeHelpButton("Help",
									  "mDNS is the host name you can use to connect to your device rather tan an IP Address which may change")
						   .c_str());

		// Base location setup
		const char *BASE_LCN = "mDNSname"; // Actual location
		if (_wifiManager.server->hasArg(BASE_LCN))
		{
			auto name = std::string(_wifiManager.server->arg(BASE_LCN).c_str());

			// Extract the name without the .local suffix
			if (name.length() > 6 && name.substr(name.length() - 6) == ".local")
				name = name.substr(0, name.length() - 6);

			// Validate the name
			bool valid = true;
			if (name.length() < 3 || name.length() > 63)
			{
				_client.println("<div class='alert alert-danger' role='alert'>Name must be between 3 and 63 characters long!</div>");
				valid = false;
			}

			// Check for invalid characters
			for (char c : name)
			{
				if (!isalnum(c) && c != '-')
				{
					_client.println("<div class='alert alert-danger' role='alert'>Name can only contain letters, numbers and hyphens!</div>");
					valid = false;
					break;
				}
			}
			// Check if the name starts or ends with a hyphen
			if (name.front() == '-' || name.back() == '-')
			{
				_client.println("<div class='alert alert-danger' role='alert'>Name cannot start or end with a hyphen!</div>");
				valid = false;
			}
			// Check if the name contains consecutive hyphens
			if (name.find("--") != std::string::npos)
			{
				_client.println("<div class='alert alert-danger' role='alert'>Name cannot contain consecutive hyphens!</div>");
				valid = false;
			}

			// All good. Save a restart
			if (valid)
			{
				_myFiles.WriteFile(MDNS_HOST_FILENAME, name.c_str());
				_client.printf("<div class='alert alert-success' role='alert'>Resetting device to set device network name to %s.local</div>", name.c_str());
				RestartDevice(_client, "/settings");
				return;
			}
		}

		// Main input form
		_client.printf(R"rawliteral(
			<form method='get' class='container py-4 m-0 p-0'>
			<div class="input-group mb-3">
				<div class="form-floating">
					<input type="text" class="form-control" name="%s" id="%s" value="%s.local">
					<label for="%s" class="form-label">Name (Use numbers, letters and hyphens)</label>
				</div>
				<button class="btn btn-primary" type='submit' id="button-addon2">Apply</button>
			</div></form>	)rawliteral",
					   BASE_LCN, BASE_LCN, _mdnsHostName.c_str(), BASE_LCN);
	}

	///////////////////////////////////////////////////////////////////////////////
	/// @brief Add the form to set DNS for the ESP32
	void AddTimezoneOffset()
	{
		// Title and help button
		_client.printf("<h3 class='mt-4'>Timezone offset %s</h3>",
					   MakeHelpButton("Help",
									  "Timezone is applied to logs. Dealing with daylight savings is just too hard. Deal with it!")
						   .c_str());

		// Base location setup
		const char *TZ_ID = "TZ_ID"; // Actual location
		if (_wifiManager.server->hasArg(TZ_ID))
		{
			auto name = std::string(_wifiManager.server->arg(TZ_ID).c_str());

			_myFiles.WriteFile(TIMEZONE_MINUTES, name.c_str());
			_client.printf("<div class='alert alert-success' role='alert'>Resetting device to set timezone %s minutes</div>", name.c_str());
			RestartDevice(_client, "/settings");
			return;
		}

		// Main input form
		_client.printf(R"rawliteral(
<form method='get' class='container py-4 m-0 p-0'>
<div class="input-group mb-3">
	<div class="form-floating">
		<select class="form-control" id="%s" name="%s">
		<option value="-720">(UTC-12:00) Baker</option>
		<option value="-660">(UTC-11:00) Pago</option>
		<option value="-600">(UTC-10:00) Hilo</option>
		<option value="-570">(UTC-09:30) Taio</option>
		<option value="-540">(UTC-09:00) Nome</option>
		<option value="-480">(UTC-08:00) LA</option>
		<option value="-420">(UTC-07:00) Denver</option>
		<option value="-360">(UTC-06:00) Chicago</option>
		<option value="-300">(UTC-05:00) Argentina</option>
		<option value="-270">(UTC-04:30) Caracas</option>
		<option value="-240">(UTC-04:00) Atlantic Time</option>
		<option value="-210">(UTC-03:30) St Johns</option>
		<option value="-180">(UTC-03:00) Reo</option>
		<option value="-120">(UTC-02:00) South Georgia</option>
		<option value="-60">(UTC-01:00) Azores</option>
		<option value="0">(UTC 00:00) London</option>
		<option value="60">(UTC+01:00) Berlin</option>
		<option value="120">(UTC+02:00) Greece</option>
		<option value="180">(UTC+03:00) Turkey</option>
		<option value="210">(UTC+03:30) Tehran</option>
		<option value="240">(UTC+04:00) Dubai</option>
		<option value="270">(UTC+04:30) Kabul</option>
		<option value="300">(UTC+05:00) Karachi</option>
		<option value="330">(UTC+05:30) India</option>
		<option value="345">(UTC+05:45) Kathmandu</option>
		<option value="360">(UTC+06:00) Dhaka</option>
		<option value="390">(UTC+06:30) Cocos Islands</option>
		<option value="420">(UTC+07:00) Thailand</option>
		<option value="480">(UTC+08:00) Singapore</option>
		<option value="525">(UTC+08:45) Perth</option>
		<option value="540">(UTC+09:00) South Korea</option>
		<option value="570">(UTC+09:30) Adelaide</option>
		<option value="600">(UTC+10:00) Brisbane</option>
		<option value="630">(UTC+10:30) Lord Howe Island</option>
		<option value="660">(UTC+11:00) Magadan</option>
		<option value="720">(UTC+12:00) New Zealand</option>
		<option value="765">(UTC+12:45) Chatham Islands</option>
		<option value="780">(UTC+13:00) Nuku</option>
  		<option value="840">(UTC+14:00) Kiritimati</option>
	</select>
		<label for="%s" class="form-label">Timezone</label>
	</div>
	<button class="btn btn-primary" type='submit' id="button-addon2">Apply</button>
</div></form>

<script>
	document.getElementById('%s').value = '%s';
</script>
)rawliteral",
					   TZ_ID, TZ_ID, TZ_ID, TZ_ID, _myFiles.LoadString(TIMEZONE_MINUTES).c_str());
	}

	///////////////////////////////////////////////////////////////////////////////
	/// @brief Form to setup a single caster
	void AddCasterForm(NTRIPServer &server)
	{
		std::string num = std::to_string(server.GetIndex() + 1);
		std::string sa = "sa" + num; // Server address parameter name
		std::string pr = "pr" + num; // Port parameter name
		std::string cr = "cr" + num; // Credential parameter name
		std::string pw = "pw" + num; // Password parameter name

		// Add wrapper for the card
		_client.println("<div class='card flex-item'>");
		_client.printf("<div class='card-header'>Caster %s %s</div>", num.c_str(),
					   server.IsEnabled() ? "" : "(Disabled)");
		_client.println("<div class='card-body p-1'>");

		// Save the new values if we have them
		bool saved = false;
		if (_wifiManager.server->hasArg(sa.c_str()) ||
			_wifiManager.server->hasArg(cr.c_str()) ||
			_wifiManager.server->hasArg(pw.c_str()))
		{
			// Check for duplicate server addresses
			std::string newAddress = _wifiManager.server->arg(sa.c_str()).c_str();

			// Remove any address formatting
			newAddress = Trim(newAddress);
			newAddress = ToLower(newAddress);

			// If the address is empty, don't check for duplicates
			if (newAddress.length() > 1)
			{
				if ((&server != &_ntripServer0 &&
					 _ntripServer0.GetAddress() == newAddress) ||
					(&server != &_ntripServer1 &&
					 _ntripServer1.GetAddress() == newAddress) ||
					(&server != &_ntripServer2 &&
					 _ntripServer2.GetAddress() == newAddress))
				{
					_client.println("<div class='alert alert-danger' role='alert'>Duplicate server address detected!</div></div></div>");
					return;
				}
			}

			// Save
			server.Save(newAddress.c_str(),
						_wifiManager.server->arg(pr.c_str()).c_str(),
						_wifiManager.server->arg(cr.c_str()).c_str(),
						_wifiManager.server->arg(pw.c_str()).c_str());

			saved = true;
		}

		// Add the form
		{
			_client.println("<form method='get' class='container py-4 m-0 p-0'>");

			AddInput("text", sa, "Server Address", server.GetAddress().c_str());
			AddInput("number", pr, "Port (0 to disable)", std::to_string(server.GetPort()).c_str());
			AddInput("text", cr, "Credential", server.GetCredential().c_str());
			AddInput("password", pw, "Password", server.GetPassword().c_str());

			if (saved)
				_client.printf("<div class='alert alert-success' role='alert'>Caster %s settings saved successfully!</div>", num.c_str());

			_client.printf("<button type='submit' class='btn btn-primary'>Save Caster %s</button></form>", num.c_str());
		}

		_client.println("</div></div>");
	}

	////////////////////////////////////////////////////////////////////////////////
	/// @brief Add an input field to the HTML page
	void AddInput(const char *type, std::string name, const char *label, const char *value)
	{
		// Display input or password depending on type
		if (strcmp(type, "password") == 0)
		{
			_client.printf(R"rawliteral(
			<div class="input-group mb-3 password-wrapper">
				<div class="form-floating">
					<input type="password" class="form-control" name="%s" id="%s" value="%s">
					<label for="%s" class="form-label">%s</label>
				</div>
				<button class="btn btn-outline-secondary toggle-password" type="button" tabindex="-1">
      				<i class="bi bi-eye-slash"></i>
	    		</button>
			</div>)rawliteral",
						   name.c_str(), name.c_str(), value, name.c_str(), label);
		}
		else
		{
			_client.printf("<div class='form-floating mb-3'>");
			_client.printf("<input type='%s' class='form-control' name='%s' id='%s' value='%s'>", type, name.c_str(), name.c_str(), value);
			_client.printf("<label for='%s' class='form-label'>%s</label>", name.c_str(), label);
			_client.println("</div>");
		}
	}

	/*
	<div class="input-group form-floating mb-3 password-wrapper">
	  <input type="password" aria-required="true" class="form-control modified valid">

	  <div class="input-group-append">
		<button class="btn btn-outline-secondary toggle-password" type="button" tabindex="-1">
		  <i class="bi bi-eye-slash"></i>
		</button>
	  </div>

	  <label class="form-label">Password</label>
	</div>*/

	// TODO : Add font awesome icons
	// TODO : Add show password option for password fields
	// _client.printf(R"rawliteral(
	// 		<div class="input-group mb-3">
	// 			<input type="text" class="form-control" aria-describedby="button-addon2">
	// 			<button class="btn btn-outline-secondary" type="button" id="button-addon2">Button</button>
	// 		</div>
	// 	)rawliteral",
	//			   name.c_str());
};
