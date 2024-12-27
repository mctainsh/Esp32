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
	//MyDisplay &_display;					// Display for updating packet count
	GNSSParser _gnssParser;					// Parser for extracting RTK
	bool _wasConnected = false;				// Was connected last time
	//MyFiles _myFiles;						// File system for saving settings
	bool _firstPacketAfterConnect = true;	// Dump the first packet after connect

	std::string _sAddress;
	int _port;
	std::string _sMountPoint;
	std::string _sUsername;
	std::string _sPassword;

	const char *SETTING_FILE = "/NtripClientSettings.txt";
public:
//	NTRIPClient(MyDisplay &display) : _display(display)
//	{
//	}

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
			//LogX(StringPrintf(" - Read config '%s'", llText.c_str()));
			auto parts = Split(llText, "\n");
			if (parts.size() > 4)
			{
				_sAddress = parts[0];
				_port = atoi(parts[1].c_str());
				_sMountPoint = parts[2];
				_sUsername = parts[3];
				_sPassword = parts[4];
				Serial.printf(" - Recovered\r\n\t Address  : %s\r\n\t Port     : %d\r\n\t Mount    : %s\r\n\t User     : %s\r\n\t Pass     : %s\r\n", _sAddress.c_str(), _port, _sMountPoint.c_str(), _sUsername.c_str(), _sPassword.c_str());
				//LogX(StringPrintf(" - Recovered\r\n\t Address  : %s\r\n\t Port     : %d\r\n\t Mid/Cred : %s\r\n\t Pass     : %s", _sAddress.c_str(), _port, _sCredential.c_str(), _sPassword.c_str()));
			}
			else
			{
				//LogX(StringPrintf(" - E341 - Cannot read saved Server settings %s", llText.c_str()));
			}
		}
		else
		{
			//LogX(StringPrintf(" - E342 - Cannot read saved Server setting %s", fileName.c_str()));
		}
	}

	void Loop()
	{
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
			if( _firstPacketAfterConnect )
			{ 
				_firstPacketAfterConnect = false;
				std::string built;
				for (int n = 0; n < buffSize; n++)
					built += static_cast<char>(_pSocketBuffer[n]);
				Serial.printf("%d %s\r\n", buffSize, built.c_str());
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

	inline const std::string GetAddress() const { return _sAddress; }
	inline const std::string GetPort() const { return std::to_string(_port); }
	inline const std::string GetMountPoint() const { return _sMountPoint; }
	inline const std::string GetUsername() const { return _sUsername; }
	inline const std::string GetPassword() const { return _sPassword; }

	//////////////////////////////////////////////////////////////////////////////
	// Save the setting to the file
	void Save(const char *address, const char *port, const char *mountPoint, const char* username, const char *password)
	{
		std::string llText = StringPrintf("%s\n%s\n%s\n%s\n%s", address, port, mountPoint, username, password);
		_myFiles.WriteFile(SETTING_FILE, llText.c_str());
	}

	////////////////////////////////////////////////////////////////////////////////
	void Reconnect()
	{
		// Limit how soon the connection is retried
		if ((millis() - _wifiConnectTime) < 10000)
			return;

		_wifiConnectTime = millis();

		// Start the connection process
		Serial.printf("RTK Not connecting to %s %d\r\n", _sAddress.c_str(), _port);
		_client.connect(_sAddress.c_str(), _port);
		if (!_client.connected())
		{
			Serial.println("E500 - RTK Not connecting");
			return;
		}

		// Send the startup credentials
		//_client.write("GET / HTTP/1.1");			// Load the source table
		_client.write(StringPrintf("GET /%s HTTP/1.1\r\n", _sMountPoint.c_str()).c_str()); // Cleveland
		_client.write(StringPrintf("Host: %s:%d\r\n", _sAddress.c_str(), _port).c_str());

		// Add credentials
		size_t output_length;
		const char* userNamePassword = ( _sUsername + ":" + _sPassword ).c_str();
		Serial.println("*******************************************************************");
		Serial.println("*******************************************************************");
		Serial.printf("'%s'\r\n",userNamePassword);
		Serial.println("*******************************************************************");
		Serial.println("*******************************************************************");
		const unsigned char* u_str = reinterpret_cast<const unsigned char*>(userNamePassword);
		//char *bearer = Base64Encode((const unsigned char *)u_str, strlen(userNamePassword), &output_length);
		auto bearer = Base64Encode(userNamePassword);
		Serial.printf("Bearer '%s'\r\n",bearer.c_str());
		if (bearer.length() > 0)
		{
			_client.write(("Authorization: Basic "+bearer+"\r\n").c_str());
			//free(bearer);
		}
		else
		{
			Serial.println("E501 - Base 64 encoding failed");
		}

		//_client.write("Accept: */*");
		_client.write("Ntrip-Version: Ntrip/2.0\r\n");
		//_client.write("Connection: Close");
		_client.write("\r\n");
	}
};