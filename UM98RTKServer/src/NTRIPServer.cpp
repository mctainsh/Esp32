#include "NTRIPServer.h"

#include <WiFi.h>

#include "HandyLog.h"
#include <GpsParser.h>

NTRIPServer::NTRIPServer(MyDisplay &display, int index,
						 const char *szAddress, int port,
						 const char *szCredential,
						 const char *szPassword)
	: _display(display), _index(index),
	  _szAddress(szAddress), _port(port),
	  _szCredential(szCredential), _szPassword(szPassword)
{
	LogX(StringPrintf("Server %s:%d %s", _szAddress, _port, _szCredential));
}

///////////////////////////////////////////////////////////////////////////////
// Loop called when we have new data to send
void NTRIPServer::Loop(const byte *pBytes, int length)
{
	// Check the index is valid
	if (_index > RTK_SERVERS)
	{
		LogX(StringPrintf("E500 - RTK Server index %d too high", index));
		return;
	}

	// Wifi check interval
	if (_client.connected())
	{
		ConnectedProcessing(pBytes, length);
	}
	else
	{
		_wasConnected = false;
		_status = "Disconn...";
		_display.RefreshRtk(_index);
		Reconnect();
	}
}

void NTRIPServer::ConnectedProcessing(const byte *pBytes, int length)
{
	if (!_wasConnected)
	{
		LogX("Connected");
		_reconnects++;
		_status = "Connected";
		_display.RefreshRtk(_index);
		_wasConnected = true;
	}

	// Send what we have received
	ConnectedProcessingSend(pBytes, length);

	// Check for new data (Not expecting much)
	ConnectedProcessingReceive();
}

//////////////////////////////////////////////////////////////////////////////
// Send the data to the RTK Caster
void NTRIPServer::ConnectedProcessingSend(const byte *pBytes, int length)
{
	if (length < 1)
		return;

	// Clear out extra send history
	if (_sendMicroSeconds.size() > 20)
		_sendMicroSeconds.erase(_sendMicroSeconds.begin());

	// Send and record time
	int startT = micros();
	int sent = _client.write(pBytes, length);
	_sendMicroSeconds.push_back(micros() - startT);

	if (sent != length)
	{
		LogX(StringPrintf("E500 - RTK Not sending all data %d of %d", sent, length));
		_client.stop();
		return;
	}
	else
	{
		LogX(StringPrintf("RTK %s Sent %d OK", _szAddress, sent));
		_wifiConnectTime = millis();
		_packetsSent++;
		_display.RefreshRtk(_index);
	}
}

//////////////////////////////////////////////////////////////////////////////
// This is usually welcome messages and errors
void NTRIPServer::ConnectedProcessingReceive()
{
	// Read the data
	int buffSize = _client.available();
	if (buffSize < 1)
		return;
	if (buffSize > SOCKET_BUFFER_MAX)
		buffSize = SOCKET_BUFFER_MAX;
	_client.read(_pSocketBuffer, buffSize);

	_pSocketBuffer[buffSize] = 0;

	// Log the data
	std::string str = "TCP IN ";
	if (GpsParser::IsAllAscii(_pSocketBuffer, buffSize))
	{
		str.append((const char *)_pSocketBuffer);
	}
	else
	{
		// Write the byte array as hex text to serial
		for (int n = 0; n < buffSize; n++)
			str.append(StringPrintf("%02x ", _pSocketBuffer[n]));
	}
	LogX(str.c_str());
}

///////////////////////////////////////////////////////////////////////////////
// Get the average send time
std::string NTRIPServer::AverageSendTime()
{
	if (_sendMicroSeconds.size() < 1)
		return "-";
	int total = 0;
	for (int n : _sendMicroSeconds)
		total += n;
	return StringPrintf("%d", total / _sendMicroSeconds.size());
}

///////////////////////////////////////////////////////////////////////////////
// Write to the debug log and keep the last few messages for display
void NTRIPServer::LogX(std::string text)
{
	Logln(text.c_str());
	_logHistory.push_back(StringPrintf("%d %s", millis(), text.c_str()));
	if (_logHistory.size() > 15)
		_logHistory.erase(_logHistory.begin());
	_display.RefreshRtkLog();
}

////////////////////////////////////////////////////////////////////////////////
void NTRIPServer::Reconnect()
{
	// Limit how soon the connection is retried
	if ((millis() - _wifiConnectTime) < 10000)
		return;

	_wifiConnectTime = millis();

	// Start the connection process
	LogX(StringPrintf("RTK Connecting to %s %d", _szAddress, _port));
	int status = _client.connect(_szAddress, _port);
	if (!_client.connected())
	{
		LogX(StringPrintf("E500 - RTK %s Not connected %d", _szAddress, status));
		return;
	}
	LogX(StringPrintf("Connected %s OK", _szAddress));

	_client.write(StringPrintf("SOURCE %s %s\r\n", _szPassword, _szCredential).c_str());
	_client.write("Source-Agent: NTRIP UM98XX/ESP32_S2_Mini\r\n");
	_client.write("STR: \r\n");
	_client.write("\r\n");
}
