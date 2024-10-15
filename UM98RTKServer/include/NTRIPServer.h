#pragma once

#define SOCKET_BUFFER_MAX 300

#include "MyDisplay.h"
#include "CredentialPrivate.h"

///////////////////////////////////////////////////////////////////////////////
// Class manages the connection to the RTK Service client
class NTRIPServer
{
public:
	NTRIPServer(MyDisplay &display)
		: _display(display)
	{
	}

	void Loop(const byte *pBytes, int length)
	{
		// Wifi check interval
		if (_client.connected())
		{
			if (!_wasConnected)
			{
				_display.SetRtkStatus("Connected");
				_display.ResetRtk();
				_wasConnected = true;
			}

			// Send what we have received
			if( length > 0)
			{
				int sent = _client.write(pBytes, length);
				if( sent != length)
				{
					Serial.printf("E500 - RTK Not sending all data %d of %d\r\n", sent, length);
					_client.stop();
					return;
				}
				else
				{
					Serial.printf("RTK Sent %d OK      \r", sent );
				}
			}


			// Check for new data
			int buffSize = _client.available();
			if (buffSize < 1)
				return;
			if (buffSize > SOCKET_BUFFER_MAX)
				buffSize = SOCKET_BUFFER_MAX;
			_client.read(_pSocketBuffer, buffSize);

			_pSocketBuffer[buffSize] = 0;
			Serial.printf("TCP IN '%s'  ->", _pSocketBuffer);
			// Write the byte array as hex text to serial
			for (int n = 0; n < buffSize; n++)
				Serial.printf("%02x ", _pSocketBuffer[n]);
			Serial.println();
			Serial.println();

			// int completePackets = _gnssParser.Parse(_pSocketBuffer, buffSize);

			// std::string built;
			// for (int n = 0; n < buffSize; n++)
			//	built += static_cast<char>(_pSocketBuffer[n]);
			// Serial.printf("%d %s\r\n", buffSize, built.c_str());
			//_display.IncrementRtkPackets(completePackets);

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

private:
	WiFiClient _client;
	uint16_t _wifiConnectTime = 0;				// Time we last had good data to prevent reconnects too fast
	byte _pSocketBuffer[SOCKET_BUFFER_MAX + 1]; // Build buffer
	MyDisplay &_display;						// Display for updating packet count
	bool _wasConnected = false;					// Was connected last time

	////////////////////////////////////////////////////////////////////////////////
	void Reconnect()
	{
		// Limit how soon the connection is retried
		if ((millis() - _wifiConnectTime) < 10000)
			return;

		_wifiConnectTime = millis();

		// Start the connection process
		Serial.printf("RTK Connecting to %s %d\r\n", ONOCOY_ADDRESS, ONOCOY_PORT);
		_client.connect(ONOCOY_ADDRESS, ONOCOY_PORT);
		if (!_client.connected())
		{
			Serial.println("E500 - RTK Not connecting");
			return;
		}
		Serial.println("Connected OK");

		_client.write(StringPrintf("SOURCE %s %s\r\n", ONOCOY_PASSWORD, ONOCOY_CREDENTIAL).c_str());
		_client.write("Source-Agent: NTRIP UM98XX/ESP32_S2_Mini\r\n");
		_client.write("STR: \r\n");
		_client.write("\r\n");
	}
};