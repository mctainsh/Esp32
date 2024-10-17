#pragma once

#define SOCKET_BUFFER_MAX 100

#include "MyDisplay.h"
#include "HandyLog.h"

///////////////////////////////////////////////////////////////////////////////
// Class manages the connection to the RTK Service client
class NTRIPServer
{
public:
	NTRIPServer(MyDisplay &display, int index,
				const char *szAddress, int port,
				const char *szCredential,
				const char *szPassword)
		: _display(display), _index(index),
		  _szAddress(szAddress), _port(port),
		  _szCredential(szCredential), _szPassword(szPassword)
	{
	}

	///////////////////////////////////////////////////////////////////////////////
	// Loop called when we have new data to send
	void Loop(const byte *pBytes, int length)
	{
		// Check the index is valid
		if (_index > RTK_SERVERS)
		{
			Logf("E500 - RTK Server index %d too high\r\n", index);
			return;
		}

		// Wifi check interval
		if (_client.connected())
		{
			if (!_wasConnected)
			{
				_display.SetRtkStatus(_index, "Connected");
				_display.ResetRtk(_index);
				_wasConnected = true;
			}

			// Send what we have received
			if (length > 0)
			{
				int sent = _client.write(pBytes, length);
				if (sent != length)
				{
					Logf("E500 - RTK Not sending all data %d of %d\r\n", sent, length);
					_client.stop();
					return;
				}
				else
				{
					Logf("RTK %s Sent %d OK\r\n", _szAddress, sent);
					_wifiConnectTime = millis();
					_display.IncrementRtkPackets(_index, 1);
				}
			}

			// Check for new data (Not expecting much)
			int buffSize = _client.available();
			if (buffSize < 1)
				return;
			if (buffSize > SOCKET_BUFFER_MAX)
				buffSize = SOCKET_BUFFER_MAX;
			_client.read(_pSocketBuffer, buffSize);

			_pSocketBuffer[buffSize] = 0;
			std::string str = "TCP IN '";
			str.append( (const char*)_pSocketBuffer );
			str += "'  ->";

			// Write the byte array as hex text to serial
			for (int n = 0; n < buffSize; n++)
				str.append( StringPrintf("%02x ", _pSocketBuffer[n]));
			Logln(str.c_str());
		}
		else
		{
			_wasConnected = false;
			_display.SetRtkStatus(_index, "Disconnected");
			Reconnect();
		}
	}

private:
	WiFiClient _client;							// Socket connection
	uint16_t _wifiConnectTime = 0;				// Time we last had good data to prevent reconnects too fast
	byte _pSocketBuffer[SOCKET_BUFFER_MAX + 1]; // Build buffer
	MyDisplay &_display;						// Display for updating packet count
	bool _wasConnected = false;					// Was connected last time
	const int _index;							// Index of the server used when updating display

	const char *_szAddress;
	const int _port;
	const char *_szCredential;
	const char *_szPassword;

	////////////////////////////////////////////////////////////////////////////////
	void Reconnect()
	{
		// Limit how soon the connection is retried
		if ((millis() - _wifiConnectTime) < 10000)
			return;

		_wifiConnectTime = millis();

		// Start the connection process
		Logf("RTK Connecting to %s %d\r\n", _szAddress, _port);
		int status = _client.connect(_szAddress, _port);
		if (!_client.connected())
		{
			Logf("E500 - RTK %s Not connected %d\r\n", _szAddress, status);
			return;
		}
		Logf("Connected %s OK\r\n", _szAddress);

		_client.write(StringPrintf("SOURCE %s %s\r\n", _szPassword, _szCredential).c_str());
		_client.write("Source-Agent: NTRIP UM98XX/ESP32_S2_Mini\r\n");
		_client.write("STR: \r\n");
		_client.write("\r\n");
	}
};