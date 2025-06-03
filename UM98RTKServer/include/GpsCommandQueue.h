#pragma once

#include <vector>
#include <string>

extern MyDisplay _display;
extern std::string _baseLocation;

///////////////////////////////////////////////////////////////////////////////
// Holds a collection of commands
// Verifies when a response matches the current queue item,
//	.. the next item is sent
//  .. command responses look like $command,version,response: OK*24
class GpsCommandQueue
{
private:
	std::vector<std::string> _strings;
	int _timeSent = 0;
	std::string _deviceType;					// Device type like UM982
	std::string _deviceFirmware = "UNKNOWN";	// Firmware version
	std::string _deviceSerial = "UNKNOWN";		// Serial number
	std::function<void(std::string)> _logToGps; // Log to GPS function
	std::string _signalGroup;					// Signal group read from config
	bool _resetProcessed = false;				// Flag used to prevent reset being sent several times
public:
	GpsCommandQueue(std::function<void(std::string)> logFunc)
	{
		_logToGps = logFunc;
#ifdef IS_LC29HDA
		_strings.push_back("PQTMVERNO");
		_strings.push_back("PQTMCFGMSGRATE,R,GGA"); // Read output rate
		_strings.push_back("PQTMCFGMSGRATE,R,GSV"); // ..
		_strings.push_back("PQTMCFGCNST,R");		// Get command set
		_strings.push_back("PQTMCFGPROT,R,1,1");	// Get output protocol
		_strings.push_back("PQTMUNIQID");			// Get unique ID
#else
		_strings.push_back("MASK");
		_strings.push_back("MODE");
		_strings.push_back("CONFIG");
		_strings.push_back("VERSION"); // Used to determine device type
//_strings.push_back("GPGGA 1");
#endif
	}

	inline const std::string &GetDeviceType() const { return _deviceType; }
	inline const std::string &GetDeviceFirmware() const { return _deviceFirmware; }
	inline const std::string &GetDeviceSerial() const { return _deviceSerial; }

	///////////////////////////////////////////////////////////////////////////
	// Start the initialise process by filling the queue
	void StartInitialiseProcess()
	{
		// Load the commands
		Logf("GPS Queue StartInitialiseProcess");
		_strings.clear();

		// Setup RTCM V3

#ifdef IS_LC29HDA
		_strings.push_back("PQTMCFGSVIN,W,1,43200,0,0,0,0");
		_strings.push_back("PAIR432,1");
		_strings.push_back("PAIR434,1");
		_strings.push_back("PAIR436,1");
		_strings.push_back("PQTMCFGMSGRATE,W,GGA,1");
#else
		//_strings.push_back("VERSION");	   // Used to determine device type
		_strings.push_back("RTCM1005 30"); // Base station antenna reference point (ARP) coordinates
		_strings.push_back("RTCM1033 30"); // Receiver and antenna description
		_strings.push_back("RTCM1077 1");  // GPS MSM7. The type 7 Multiple Signal Message format for the USA’s GPS system, popular.
		_strings.push_back("RTCM1087 1");  // GLONASS MSM7. The type 7 Multiple Signal Message format for the Russian GLONASS system.
		_strings.push_back("RTCM1097 1");  // Galileo MSM7. The type 7 Multiple Signal Message format for Europe’s Galileo system.
		_strings.push_back("RTCM1117 1");  // QZSS MSM7. The type 7 Multiple Signal Message format for Japan’s QZSS system.
		_strings.push_back("RTCM1127 1");  // BeiDou MSM7. The type 7 Multiple Signal Message format for China’s BeiDou system.
		_strings.push_back("RTCM1137 1");  // NavIC MSM7. The type 7 Multiple Signal Message format for India’s NavIC system.

		// Setup base station mode
		if (_baseLocation.empty())
			_strings.push_back("MODE BASE TIME 600 1"); // Set base mode with 600 second startup and 1m optimized save error
		else
			_strings.push_back("MODE BASE " + _baseLocation); // Set base mode with location
		// It is better to workout your exact location and set it here as latitude, longitude, height
		//_strings.push_back("MODE BASE  latitude longitude height");

		// TODO :Only ever save once to protect the flash
		_strings.push_back("SAVECONFIG");
#endif

		SendTopCommand();
	}

	///////////////////////////////////////////////////////////////////////////
	/// @brief Process a line of text from the GPS unit checking got version information
	///		'#VERSION,0,GPS,UNKNOWN,0,0,0,0,0,1261;UM982,R4.10Build11826,HRPT00-S10C-P,2310415000012-LR23A0225104240,ff27289609cf869d,2023/11/24*4d0ec3ba'
	//  	'#VERSION,96,GPS,FINE,2359,203543000,0,0,18,326;UM982,R4.10Build11826,HRPT00-S10C-P,2310415000012-LR23A0225105243,ff279b9611292f1d,2023/11/24*e84c236b'
	inline void CheckForVersion(const std::string &str)
	{
		if (!StartsWith(str, "#VERSION"))
			return;

		auto sections = Split(str, ";");
		if (sections.size() < 1)
		{
			Logf("DANGER 301 : Unknown sections '%s' Detected", str.c_str());
			return;
		}

		auto parts = Split(sections[1], ",");
		if (parts.size() < 5)
		{
			Logf("DANGER 302 : Unknown split '%s' Detected", str.c_str());
			return;
		}
		_deviceType = parts[0];
		_deviceFirmware = parts[1];
		auto serialPart = Split(parts[3], "-");
		_deviceSerial = serialPart[0];
		_display.RefreshScreen();

		// New documentation for Unicore. The new firmware (Build17548) has 50 Hz and QZSS L6 reception instead of Galileo E6.
		// .. From now on, we install the Build17548 firmware on all new UM980 receivers. So we have a new advantage - you can
		// .. enable L6 instead of E6 by changing SIGNALGOUP from 2 to 10. Another thing is that this is only needed in Japan
		// .. and countries close to it.

		// Setup signal groups
		std::string command = "CONFIG SIGNALGROUP 3 6"; // Assume for UM982)
		if (_deviceType == "UM982")
		{
			Logln("UM982 Detected");
		}
		else if (_deviceType == "UM980")
		{
			Logln("UM980 Detected");
			command = "CONFIG SIGNALGROUP 2"; // (for UM980)
		}
		else
		{
			Logf("E301 Unknown Device '%s' Detected in %s", _deviceType.c_str(), str.c_str());
		}

		// If current signal group is different then set it
		if (_signalGroup != command)
		{
			_strings.push_back(command);
		}
		else
		{
			Logf("GPS : Signal Group '%s' already set", _signalGroup.c_str());
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////
	// Get the GPS checksum
	inline unsigned char CalculateChecksum(const std::string &data)
	{
		unsigned char checksum = 0;
		for (char c : data)
			checksum ^= c;
		return checksum;
	}
	inline bool VerifyChecksum(const std::string &str)
	{
		size_t asterisk_pos = str.find_last_of('*');
		if (asterisk_pos == std::string::npos || asterisk_pos + 3 > str.size())
			return false; // Invalid format

		// Extract the data and the checksum
#ifdef IS_LC29HDA
		int startIndex = 1; // Skip the first '$' character
#else
		int startIndex = 0; // No need to skip the first '$' character
#endif
		std::string data = str.substr(startIndex, asterisk_pos - startIndex);
		std::string provided_checksum_str = str.substr(asterisk_pos + 1, 2);

		// Convert the provided checksum from hex to an integer
		unsigned int provided_checksum;
		std::stringstream ss;
		ss << std::hex << provided_checksum_str;
		ss >> provided_checksum;

		// Calculate the checksum of the data
		unsigned char calculated_checksum = CalculateChecksum(data);

		return calculated_checksum == static_cast<unsigned char>(provided_checksum);
	}

	// ////////////////////////////////////////////////////////////////////////
	// Check if the first item in the list matches the given string
	bool IsCommandResponse(const std::string &str)
	{
		if (_strings.empty())
			return false; // List is empty, no match

		std::string match = "$command,";

		// Check it start correctly
		if (str.find(match) != 0)
			return false;

		// Verify checksum
		if (!VerifyChecksum(str))
		{
			Logf("GPS Checksum error in %s", str.c_str());
			return false;
		}

		// Check for a command match
		match += _strings.front();
		match += ",response: OK*";

		if (str.compare(0, match.size(), match) != 0)
			return false;

		// Clear the sent command
		_strings.erase(_strings.begin());

		if (_strings.empty())
		{
			Logf("GPS Startup Commands Complete");
		}

		// Send next command
		SendTopCommand();
		return true;
	}

	///////////////////////////////////////////////////////////////////////////
	// Check if the GPS receiver has reset itself and send all the commands
	// .. reset command looks like "$devicename,COM1*67"
	bool HasDeviceReset(const std::string &str)
	{
		const std::string match = "$devicename,COM";
		bool isReset = str.compare(0, match.size(), match) == 0;

		if (_resetProcessed)
			return isReset;

		if (!isReset)
			return false;

		// Depending on the reset count we send different parameters
		// First reset we set the signal group (This will only reset if SG changes)
		// Second reset we set the RTCM3 messages
		// Third reset we save config

		_display.UpdateGpsStarts(false, true);
		StartInitialiseProcess();
		_resetProcessed = true;
		return true;
	}

	///////////////////////////////////////////////////////////////////////////
	// Check if the we got a config response and save the current setting
	// 		'$CONFIG,SIGNALGROUP,CONFIG SIGNALGROUP 3 6*01'
	// 		'$CONFIG,ANTENNA,CONFIG ANTENNA POWERON*7A'
	bool IsConfigResponse(const std::string &str)
	{
		const std::string match = "$CONFIG,";
		if (str.compare(0, match.size(), match) != 0)
			return false;

		// Remove the checksum
		auto body = Split(str, "*");
		if (body.size() < 2)
		{
			Logf("E303 : Unknown split '%s' Detected", str.c_str());
			return false;
		}

		// Extract the config type
		auto parts = Split(body[0], ",");
		if (parts.size() < 3)
		{
			Logf("E304 : Unknown split '%s' Detected", str.c_str());
			return false;
		}

		// If config type is "SIGNALGROUP" then we need to set the RTCM3 messages
		if (parts[1] == "SIGNALGROUP")
		{
			_signalGroup = parts[2];
			Logf("GPS : Detected '%s'", _signalGroup.c_str());
		}
		return true;
	}

	///////////////////////////////////////////////////////////////////////////
	// Issue RESET command
	void IssueFReset()
	{
#ifdef IS_LC29HDA
		_strings.push_back("PQTMSRR");
#else
		_strings.push_back("FRESET");
#endif
		SendTopCommand();
	}

	//////////////////////////////////////////////////////////////////////////
	// Check queue for timeouts
	void CheckForTimeouts()
	{
		if (_strings.empty())
			return;

		if ((millis() - _timeSent) > 8000)
		{
			Logf("E940 - Timeout on %s", _strings.front().c_str());
			SendTopCommand();
		}
	}

	///////////////////////////////////////////////////////////////////////////
	/// @brief LC29H packet processing
	/// @param str The input string to process
	/// @return True if the packet is processed successfully, false otherwise
	bool ProcessLC29H(const std::string &str)
	{
		if (!VerifyChecksum(str))
		{
			Logf("ERROR : GPS Checksum error in %s", str.c_str());
			return false;
		}

		// Check for command match
		std::string match = "$" + _strings.front();

		if (StartsWith(str, "$PQTMCFGSVIN,OK") ||
			StartsWith(str, "$PQTMCFGMSGRATE,OK") ||
			StartsWith(str, "$PQTMCFGCNST,OK") ||
			StartsWith(str, "$PQTMCFGPROT,") ||
			StartsWith(str, "$PQTMUNIQID,"))
		{
			Logf("GPS Configured : %s", str.c_str());
		}
		else if (StartsWith(str, "$PQTMCFGSVIN"))
		{
			Logf("ERROR GPS NOT Configured : %s", str.c_str());
		}
		else if (StartsWith(str, "$PQTMVERNO") && str.length() > 30)
		{
			if (!ExtractLC29HVersion(str))
				return false;
		}
		else if (StartsWith(str, "$PAIR432"))
		{
			if (str.length() < 15)
			{
				Logf("ERROR : PAIR432 too short %s", str.c_str());
				return false;
			}
			if (match.length() < 10)
			{
				Logf("ERROR : %s too short for PAIR", str.c_str());
				return false;
			}
			if (match[5] != str[9] || match[6] != str[10] || match[7] != str[11])
			{
				Logf("ERROR : PAIR432 mismatch %s and %s", str.c_str(), match.c_str());
				return false;
			}
		}
		else if (StartsWith(str, "$PAIR001"))
		{
			if (str.length() < 15)
			{
				Logf("ERROR : PAIR001 too short %s", str.c_str());
				return false;
			}
			if (match.length() < 10)
			{
				Logf("ERROR : %s too short for PAIR", str.c_str());
				return false;
			}
			if (match[5] != str[9] || match[6] != str[10] || match[7] != str[11])
			{
				Logf("ERROR : PAIR001 mismatch %s and %s", str.c_str(), match.c_str());
				return false;
			}
		}
		else
		{
			if (!StartsWith(str, match))
				return false;
		}

		_strings.erase(_strings.begin());

		if (_strings.empty())
			Logln("GPS Startup Commands Complete");

		SendTopCommand();
		return true;
	}

	///////////////////////////////////////////////////////////////////////////
	// Extract the LC29H version information from the string
	//		'$PQTMVERNO,LC29HDANR11A03S_RSA,2024/03/19,13:55:31*20'
	bool ExtractLC29HVersion(const std::string &str)
	{
		// Get part between the first and second comma
		auto firstComma = str.find(',');
		auto secondComma = str.find(',', firstComma + 1);
		if (firstComma == std::string::npos || secondComma == std::string::npos)
		{
			Logf("ERROR : PQTMVERNO format error %s", str.c_str());
			return false;
		}
		auto part = str.substr(firstComma + 1, secondComma - firstComma - 1);
		if (part.length() < 10)
		{
			Logf("ERROR : PQTMVERNO part too short %s", str.c_str());
			return false;
		}
		_deviceType = part.substr(0, 7);
		Logf("GPS Device Type : %s", _deviceType.c_str());
		_deviceFirmware = part.substr(7);
		Logf("GPS Firmware Version : %s", _deviceFirmware.c_str());
		return true;
	}

	///////////////////////////////////////////////////////////////////////////////////////
	// Send the command and check set timeouts
	void SendTopCommand()
	{
		if (_strings.empty())
			return;
		_logToGps("GPS -> " + _strings.front());

#ifdef IS_LC29HDA
		std::string command = _strings.front();
		unsigned char checksum = CalculateChecksum(command);
		std::stringstream ss;
		ss << std::hex << std::uppercase << (checksum < 16 ? "0" : "") << static_cast<int>(checksum);
		std::string checksumHex = ss.str();
		std::string finalCommand = "$" + command + "*" + checksumHex + "\r\n";
		Serial2.print(finalCommand.c_str());
#else
		Serial2.println(_strings.front().c_str());
#endif
		_timeSent = millis();
	}
};
