#include "NTRIPServer.h"

#include <WiFi.h>

#include "HandyLog.h"
#include <GpsParser.h>
#include <MyFiles.h>

extern MyFiles _myFiles;

NTRIPServer::NTRIPServer(MyDisplay &display, int index)
	: _display(display), _index(index)
{
	_sendMicroSeconds.reserve(AVERAGE_SEND_TIMERS);
}

//////////////////////////////////////////////////////////////////////////////
// Load the configurations if they exist
void NTRIPServer::LoadSettings()
{
	std::string fileName = StringPrintf("/Caster%d.txt", _index);
	// Read the server settings from the config file
	// Load last lat long
	std::string llText;
	if (_myFiles.ReadFile(fileName.c_str(), llText))
	{
		LogX(StringPrintf(" - Read config '%s'", llText.c_str()));
		auto parts = Split(llText, "\n");
		if (parts.size() > 3)
		{
			_sAddress = parts[0];
			_port = atoi(parts[1].c_str());
			_sCredential = parts[2];
			_sPassword = parts[3];
			LogX(StringPrintf(" - Recovered\r\n\t Address  : %s\r\n\t Port     : %d\r\n\t Mid/Cred : %s\r\n\t Pass     : %s", _sAddress.c_str(), _port, _sCredential.c_str(), _sPassword.c_str()));
		}
		else
		{
			LogX(StringPrintf(" - E341 - Cannot read saved Server settings %s", llText.c_str()));
		}
	}
	else
	{
		LogX(StringPrintf(" - E342 - Cannot read saved Server setting %s", fileName.c_str()));
	}
}

//////////////////////////////////////////////////////////////////////////////
// Save the setting to the file
void NTRIPServer::Save(const char *address, const char *port, const char *credential, const char *password) const
{
	std::string llText = StringPrintf("%s\n%s\n%s\n%s", address, port, credential, password);
	std::string fileName = StringPrintf("/Caster%d.txt", _index);
	_myFiles.WriteFile(fileName.c_str(), llText.c_str());
}

///////////////////////////////////////////////////////////////////////////////
// Loop called when we have new data to send
void NTRIPServer::Loop(const byte *pBytes, int length)
{
	// Disable the port if not used
	if (_port < 1 || _sAddress.length() < 1)
	{
		_status = "Disabled";
		_display.RefreshRtk(_index);
		return;
	}

	// Check the index is valid
	if (_index > RTK_SERVERS)
	{
		LogX(StringPrintf("E501 - RTK Server index %d too high", index));
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
	while (_sendMicroSeconds.size() >= AVERAGE_SEND_TIMERS)
		_sendMicroSeconds.erase(_sendMicroSeconds.begin());

	// Send and record time
	unsigned long startT = micros();
	int sent = _client.write(pBytes, length);

	unsigned long time = micros() - startT;
	if (_maxSendTime == 0)
		_maxSendTime = time;
	else
		_maxSendTime = max(_maxSendTime, time);

	if (sent != length)
	{
		LogX(StringPrintf("E500 - %s Only sent %d of %d", _sAddress.c_str(), sent, length));
		_client.stop();
	}
	else
	{
		// Logf("RTK %s Sent %d OK", _sAddress.c_str(), sent);
		_sendMicroSeconds.push_back(sent * 8 * 1000 / max(1UL, time));
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

	buffSize = min(buffSize, SOCKET_IN_BUFFER_MAX);

	std::unique_ptr<byte[]> buffer(new byte[buffSize + 1]);
	auto _pSocketBuffer = buffer.get();
	_client.read(_pSocketBuffer, buffSize);

	_pSocketBuffer[buffSize] = 0;

	// Log the data
	LogX("IN -> " + _sAddress + "\r\n" + HexAsciDump(_pSocketBuffer, buffSize));
}

///////////////////////////////////////////////////////////////////////////////
// Get the average send time
int NTRIPServer::AverageSendTime()
{
	if (_sendMicroSeconds.size() < 1)
		return 0;
	int total = 0;
	for (int n : _sendMicroSeconds)
		total += n;
	return total / _sendMicroSeconds.size();
}

///////////////////////////////////////////////////////////////////////////////
// Write to the debug log and keep the last few messages for display
void NTRIPServer::LogX(std::string text)
{
	auto s = Logln(text.c_str());
	_logHistory.push_back(s);
	TruncateLog(_logHistory);
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
	LogX(StringPrintf("RTK Connecting to %s : %d", _sAddress.c_str(), _port));
	int status = _client.connect(_sAddress.c_str(), _port);
	if (!_client.connected())
	{
		LogX(StringPrintf("E500 - RTK %s Not connected %d", _sAddress.c_str(), status));
		return;
	}
	LogX(StringPrintf("Connected %s OK", _sAddress.c_str()));

	_client.write(StringPrintf("SOURCE %s %s\r\n", _sPassword.c_str(), _sCredential.c_str()).c_str());
	_client.write("Source-Agent: NTRIP UM98XX/ESP32_T_Display_SX\r\n");
	_client.write("STR: \r\n");
	_client.write("\r\n");
}
