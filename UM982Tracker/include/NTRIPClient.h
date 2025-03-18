#pragma once

#define SOCKET_BUFFER_MAX 300

#include "GNSSParser.h"
#include "MyFiles.h"
#include "HandyString.h"
#include "MyDisplay.h"

extern MyFiles _myFiles;
extern MyDisplay _display;

///////////////////////////////////////////////////////////////////////////////
// Class manages the connection to the RTK Service client
class NTRIPClient
{
private:
	WiFiClient _client;
	uint16_t _wifiConnectTime = 0;			// Time we last had good data to prevent reconnects too fast
	byte _pSocketBuffer[SOCKET_BUFFER_MAX]; // Build buffer
	GNSSParser _gnssParser;					// Parser for extracting RTK
	bool _wasConnected = false;				// Was connected last time
	bool _firstPacketAfterConnect = true;	// Dump the first packet after connect
	std::string _sAddress;					// ntrip.data.gnss.ga.gov.au
	int _port;								// 2101
	std::string _sMountPoint;				// CLEV00AUS0
	std::string _sUsername;
	std::string _sPassword;

	const char *SETTING_FILE = "/NtripClientSettings.txt";

public:
	//////////////////////////////////////////////////////////////////////////////
	// Load the configurations if they exist
	void LoadSettings()
	{
		// Load default settings
		_sAddress = "ntrip.data.gnss.ga.gov.au";
		_port = 2101;
		_sMountPoint = "CLEV00AUS0";
		_sUsername = "mctainsh";
		_sPassword = "";

		// Read the server settings from the config file
		std::string llText;
		if (_myFiles.ReadFile(SETTING_FILE, llText))
		{
			// LogX(StringPrintf(" - Read config '%s'", llText.c_str()));
			auto parts = Split(llText, "\n");
			if (parts.size() > 4)
			{
				_sAddress = parts[0];
				_port = atoi(parts[1].c_str());
				_sMountPoint = parts[2];
				_sUsername = parts[3];
				_sPassword = parts[4];
				Logf(" - Recovered\r\n\t Address  : %s\r\n\t Port     : %d\r\n\t Mount    : %s\r\n\t User     : %s\r\n\t Pass     : %s", _sAddress.c_str(), _port, _sMountPoint.c_str(), _sUsername.c_str(), _sPassword.c_str());
				_display.SetRtkStatus("PENDING");
				// LogX(StringPrintf(" - Recovered\r\n\t Address  : %s\r\n\t Port     : %d\r\n\t Mid/Cred : %s\r\n\t Pass     : %s", _sAddress.c_str(), _port, _sCredential.c_str(), _sPassword.c_str()));
			}
			else
			{
				_display.SetRtkStatus("E341:BAD SETTINGS");
				// LogX(StringPrintf(" - E341 - Cannot read saved Server settings %s", llText.c_str()));
			}
		}
		else
		{
			_display.SetRtkStatus("E342:NO SETTINGS");
			// LogX(StringPrintf(" - E342 - Cannot read saved Server setting %s", fileName.c_str()));
		}
		if (_port == 0)
			_display.SetRtkStatus("DISABLED");
	}

	void Loop()
	{
		// Disable RTK if port is not set
		if (_port == 0)
			return;

		// Wifi check interval
		if (_client.connected())
		{
			if (!_wasConnected)
			{
				_display.SetRtkStatus("Connected");
				_display.ResetRtk();
				_wasConnected = true;
				_firstPacketAfterConnect = true;
			}
			// Check for new data
			int buffSize = _client.available();
			if (buffSize < 1)
				return;
			if (buffSize > SOCKET_BUFFER_MAX)
				buffSize = SOCKET_BUFFER_MAX;
			_client.read(_pSocketBuffer, buffSize);

			// Testing dump text
			if (_firstPacketAfterConnect)
			{
				_firstPacketAfterConnect = false;
				std::string built;
				for (int n = 0; n < buffSize; n++)
					built += static_cast<char>(_pSocketBuffer[n]);
				Logf("%d %s", buffSize, built.c_str());
			}

			int completePackets = _gnssParser.Parse(_pSocketBuffer, buffSize);

			_display.IncrementRtkPackets(completePackets);

			// record we had some data so do not try to reconnect straight away
			_wifiConnectTime = millis();
		}
		else
		{
			_wasConnected = false;
			_display.SetRtkStatus("Disconnected");
			Reconnect();
		}
	}

	//////////////////////////////////////////////////////////////////////////////
	// Called when the NTRIP settings page is to be displayed
	void DisplaySettings()
	{
		_display.DrawLabel("Wi-Fi", COL1, R1F4, 2);
		_display.DrawLabel("RTK", COL1, R2F4, 2);
		_display.DrawLabel("Mount", COL1, R3F4, 2);
		_display.DrawLabel("Destin", COL1, R4F4, 2);

		_display.DrawML(GetAddress().c_str(), COL2_P0, R1F4, COL2_P0_W, 4);
		_display.DrawML(GetPort().c_str(), COL2_P0, R2F4, COL2_P0_W, 4);
		_display.DrawML(GetMountPoint().c_str(), COL2_P0, R3F4, COL2_P0_W, 4);
		_display.DrawML(GetUsername().c_str(), COL2_P0, R4F4, COL2_P0_W, 4);
	}

	inline const std::string GetAddress() const { return _sAddress; }
	inline const std::string GetPort() const { return std::to_string(_port); }
	inline const std::string GetMountPoint() const { return _sMountPoint; }
	inline const std::string GetUsername() const { return _sUsername; }
	inline const std::string GetPassword() const { return _sPassword; }

	//////////////////////////////////////////////////////////////////////////////
	// Save the setting to the file
	void Save(const char *address, const char *port, const char *mountPoint, const char *username, const char *password)
	{
		std::string llText = StringPrintf("%s\n%s\n%s\n%s\n%s", address, port, mountPoint, username, password);
		_myFiles.WriteFile(SETTING_FILE, llText.c_str());
	}

	////////////////////////////////////////////////////////////////////////////////
	// Reconnect to the NTRIP server
	void Reconnect()
	{
		// Limit how soon the connection is retried
		if ((millis() - _wifiConnectTime) < 30000)
			return;

		_wifiConnectTime = millis();

		// Start the connection process
		Logln("*******************************************************************");
		Logf("RTK Not connecting to %s : %d", _sAddress.c_str(), _port);
		_client.connect(_sAddress.c_str(), _port);
		if (!_client.connected())
		{
			Logln("E500 - RTK Not connecting");
			return;
		}

		// Send the startup credentials
		//_client.write("GET / HTTP/1.1");			// Load the source table
		_client.write(StringPrintf("GET /%s HTTP/1.1\r\n", _sMountPoint.c_str()).c_str()); // Cleveland
		_client.write(StringPrintf("Host: %s:%d\r\n", _sAddress.c_str(), _port).c_str());

		// Add credentials
		size_t output_length;
		std::string userNamePassword = (_sUsername + ":" + _sPassword);
		Logf("'%s'", userNamePassword.c_str());
		//const unsigned char *u_str = reinterpret_cast<const unsigned char *>(userNamePassword.c_str());
		// char *bearer = Base64Encode((const unsigned char *)u_str, strlen(userNamePassword), &output_length);
		std::string bearer = Base64Encode(userNamePassword);
		Logf("Bearer '%s'", bearer.c_str());
		if (bearer.length() > 0)
		{
			_client.write(("Authorization: Basic " + bearer + "\r\n").c_str());
			// free(bearer);
		}
		else
		{
			Logln("E501 - Base 64 encoding failed");
		}

		//_client.write("Accept: */*");
		_client.write("Ntrip-Version: Ntrip/2.0\r\n");
		//_client.write("Connection: Close");
		_client.write("\r\n");
	}
};