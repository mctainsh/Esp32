#pragma once

#include <WiFi.h>

// Buffer used to grab data to send
#define SOCKET_IN_BUFFER_MAX 512

// Number of items in the averaging buffer for send time calculation
#define AVERAGE_SEND_TIMERS 256

#include <string>
#include <vector>
#include "QueueData.h"

///////////////////////////////////////////////////////////////////////////////
// Class manages the connection to the RTK Service client
class NTRIPServer
{
public:
	NTRIPServer(int index);
	void LoadSettings();
	void Save(const char *address, const char *port, const char *credential, const char *password) const;
	bool EnqueueData(const byte *pBytes, int length);
	std::vector<std::string> GetLogHistory();
	const char *GetStatus() const;

	inline const int GetIndex() const { return _index; }
	inline int GetReconnects() const { return _reconnects; }
	inline int GetPacketsSent() const { return _packetsSent; }
	inline const std::string GetAddress() const { return _sAddress; }
	inline int GetPort() const { return _port; }
	inline const std::string GetCredential() const { return _sCredential; }
	inline const std::string GetPassword() const { return _sPassword; }
	inline int GetMaxSendTime() const { return _maxSendTime; }
	inline UBaseType_t GetMaxStackHeight() const { return _maxStackHeight; }
	inline unsigned long GetQueueOverflows() const { return _queueOverflows; }
	inline unsigned long GetExpiredPackets() const { return _expiredPackets; }
	inline unsigned long GetTotalTimeouts() const { return _totalTimeouts; }
	void TaskFunction();

	enum class ConnectionState
	{
		Unknown,
		Disabled,
		Connected,
		Disconnected,
	};

private:
	WiFiClient _client;									// Socket connection
	unsigned long _wifiConnectTime = 0;					// Time we last had good data to prevent reconnects too fast
	bool _wasConnected = false;							// Was connected last time
	const int _index;									// Index of the server used when updating display
	ConnectionState _status = ConnectionState::Unknown; // Connection status
	std::vector<std::string> _logHistory;				// History of connection status
	int _reconnects;									// Total number of reconnects
	int _packetsSent;									// Total number of packets sent
	unsigned long _maxSendTime;							// Maximum amount of time it took to send a packet
	unsigned long _queueOverflows = 0;					// Number of packets dropped from the queue
	unsigned long _lastStackCheck = 0;					// Last time we checked the stack height
	UBaseType_t _maxStackHeight = 0;					// Stack height
	unsigned long _expiredPackets = 0;					// Number of packets that were expired
	int _overflowSetSize = 0;							// Number of times the overflow set was used in single set
	int _timeOutIndex = 0;								// Index even increasing timeout periods
	int _totalTimeouts = 0;								// Total number of timeouts
 	int _consecutiveTimeouts = 0;						// Number of consecutive timeouts

	std::string _sAddress;
	int _port;
	std::string _sCredential;
	std::string _sPassword;

	const SemaphoreHandle_t _logMutex;	 // Thread safe log access
	const SemaphoreHandle_t _queMutex;	 // Thread safe queue access
	std::vector<QueueData *> _dataQueue; // Queue to hold QueueData items
	TaskHandle_t _connectingTask = NULL; // Task handle for the main connection and sending task

	QueueData *DequeueData();
	void ConnectedProcessing(const byte *pBytes, int length);
	void ConnectedProcessingSend(const byte *pBytes, int length);
	void ConnectedProcessingReceive();
	void LogX(std::string text);
	bool Reconnect();
	bool WriteText(const char *str);
};