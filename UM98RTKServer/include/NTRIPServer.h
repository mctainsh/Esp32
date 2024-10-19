#pragma once

#define SOCKET_BUFFER_MAX 100

#include <string>
#include <vector>

#include "MyDisplay.h"

///////////////////////////////////////////////////////////////////////////////
// Class manages the connection to the RTK Service client
class NTRIPServer
{
public:
	NTRIPServer(MyDisplay &display, int index,
				const char *szAddress, int port,
				const char *szCredential,
				const char *szPassword);
	void Loop(const byte *pBytes, int length);
	std::string AverageSendTime();

	inline const std::vector<std::string> &GetLogHistory() const { return _logHistory; }
	inline const char *GetStatus() const { return _status; }
	inline int GetReconnects() const { return _reconnects; }
	inline int GetPacketsSent() const { return _packetsSent; }
	inline const char *GetAddress() const { return _szAddress; }

private:
	WiFiClient _client;							// Socket connection
	uint16_t _wifiConnectTime = 0;				// Time we last had good data to prevent reconnects too fast
	byte _pSocketBuffer[SOCKET_BUFFER_MAX + 1]; // Build buffer
	MyDisplay &_display;						// Display for updating packet count
	bool _wasConnected = false;					// Was connected last time
	const int _index;							// Index of the server used when updating display
	const char *_status = "-";					// Connection status
	std::vector<std::string> _logHistory;		// History of connection status
	std::vector<int> _sendMicroSeconds;			// Collection of send times
	int _reconnects;							// Total number of reconnects
	int _packetsSent;							// Total number of packets sent

	const char *_szAddress;
	const int _port;
	const char *_szCredential;
	const char *_szPassword;

	void ConnectedProcessing(const byte *pBytes, int length);
	void ConnectedProcessingSend(const byte *pBytes, int length);
	void ConnectedProcessingReceive();
	void LogX(std::string text);
	void Reconnect();
};