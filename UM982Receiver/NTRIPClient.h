#pragma once

#define SOCKET_BUFFER_MAX 300

#include <WiFi.h>

#include "GNSSParser.h"

///////////////////////////////////////////////////////////////////////////////
// Class manages the connection to the RTK Service client
class NTRIPClient
{
  public:
	NTRIPClient(MyDisplay& display)
	  : _display(display)
	{
	}

	void Loop()
	{
		// Wifi check interval
		if (_client.connected())
		{
			// Check for new data
			int buffSize = _client.available();
			if (buffSize < 1)
				return;
			if (buffSize > SOCKET_BUFFER_MAX)
				buffSize = SOCKET_BUFFER_MAX;
			_client.read(_pSocketBuffer, buffSize);

			int completePackets = _gnssParser.Parse(_pSocketBuffer, buffSize);

			//std::string built;
			//for (int n = 0; n < buffSize; n++)
			//	built += static_cast<char>(_pSocketBuffer[n]);
			//Serial.printf("%d %s\r\n", buffSize, built.c_str());
			_display.IncrementRtkPackets(completePackets);
		}
		else
		{
			Reconnect();
		}
	}
  private:
	WiFiClient _client;
	uint16_t _wifiReconnectTime = 0;
	byte _pSocketBuffer[SOCKET_BUFFER_MAX];
	MyDisplay& _display;
	GNSSParser _gnssParser;

	////////////////////////////////////////////////////////////////////////////////
	void Reconnect()
	{
		// Limit how soon the connection is retried
		if( (millis() - _wifiReconnectTime) < 10000 )
			return;

		_wifiReconnectTime = millis();

		// Start the connection process
		Serial.printf("RTK Not connecting to %s %d\r\n", RTF_SERVER_ADDRESS, RTF_SERVER_PORT);
		_client.connect(RTF_SERVER_ADDRESS, RTF_SERVER_PORT);
		if (!_client.connected())
		{
			Serial.println("E500 - RTK Not connecting");
			return;
		}

		// Send the startup credentials
		//_client.write("GET / HTTP/1.1");			// Load the source table
		_client.write("GET /CLEV00AUS0 HTTP/1.1\r\n");	// Cleveland
														//_client.write("GET /PTHL00AUS0 HTTP/1.1");       // Port headland
		_client.write(StringPrintf("Host: %s:%d\r\n", RTF_SERVER_ADDRESS, RTF_SERVER_PORT).c_str());

		// Add credentials
		size_t output_length;
		char* bearer = Base64Encode((const unsigned char*)RTK_USERNAME_PASSWORD, strlen(RTK_USERNAME_PASSWORD), &output_length);
		if (bearer)
		{
			_client.write(StringPrintf("Authorization: Basic %s\r\n", bearer).c_str());
			free(bearer);
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