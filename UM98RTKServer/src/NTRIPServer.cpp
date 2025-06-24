#include "NTRIPServer.h"

#include <WiFi.h>

#include "HandyLog.h"
#include <GpsParser.h>
#include <MyFiles.h>
#include "History.h"

extern MyFiles _myFiles;
extern History _history;

// Progressive time out for reconnecting
static const unsigned long WIFI_TIMEOUTS[] = {15000, 30000, 60000, 120000, 300000};
static const int WIFI_TIMEOUT_SIZE = sizeof(WIFI_TIMEOUTS) / sizeof(WIFI_TIMEOUTS[0]);

///////////////////////////////////////////////////////////////////////////////
// Constructor
NTRIPServer::NTRIPServer(int index)
	: _index(index),
	  _logMutex(xSemaphoreCreateMutex()),
	  _queMutex(xSemaphoreCreateMutex())
{
	_wifiConnectTime = 10000 - WIFI_TIMEOUTS[_timeOutIndex]; // Start the connection process after 10 seconds

	// Check mutexs
	if (_logMutex == nullptr)
		perror("Failed to create log mutex\n");
	else
		Serial.printf("Log %d Mutex Created\r\n", index);
	if (_queMutex == nullptr)
		perror("Failed to create queue mutex\n");
	else
		Serial.printf("Queue %d Mutex Created\r\n", index);
}

static void TaskWrapper(void *param)
{
	// Cast the parameter back to MyClass and call the member function
	NTRIPServer *instance = static_cast<NTRIPServer *>(param);
	instance->TaskFunction();
}

//////////////////////////////////////////////////////////////////////////////
// Load the configurations if they exist
void NTRIPServer::LoadSettings()
{
	std::string fileName = StringPrintf("/Caster%d.txt", _index);

	// Read the server settings from the config file
	std::string llText;
	if (_myFiles.ReadFile(fileName.c_str(), llText))
	{
		LogX(StringPrintf(" - Read config [%d] '%s'", llText.length(), llText.c_str()));
		auto parts = Split(llText, "\n");
		if (parts.size() > 3)
		{
			_sAddress = parts[0];
			_port = atoi(parts[1].c_str());
			_sCredential = parts[2];
			_sPassword = parts[3];
			LogX(StringPrintf(" - Recovered\r\n\t Address  : '%s'\r\n\t Port     : %d\r\n\t Mpt/Cred : '%s'\r\n\t Pass     : '%s'", _sAddress.c_str(), _port, _sCredential.c_str(), _sPassword.c_str()));
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

	// Don't start thread if disabled
	if (_port < 1 || _sAddress.length() < 1)
	{
		LogX(StringPrintf(" - E343 - Server %d is disabled", _index));
		_status = ConnectionState::Disabled;
		return;
	}

	// Check the index is valid
	if (_index > RTK_SERVERS)
	{
		LogX(StringPrintf("E501 - RTK Server index %d too high", index));
		return;
	}

	// Start the connection process
	xTaskCreatePinnedToCore(
		TaskWrapper,
		StringPrintf("NtripSvrTask%d", index).c_str(), // Task name
		5000,										   // Stack size (bytes)
		this,										   // Parameter
		1,											   // Task priority
		&_connectingTask,							   // Task handle
		APP_CPU_NUM);
}

//////////////////////////////////////////////////////////////////////////////
// Save the setting to the file
void NTRIPServer::Save(const char *address, const char *port, const char *credential, const char *password)
{
	std::string llText = StringPrintf("%s\n%s\n%s\n%s", address, port, credential, password);
	std::string fileName = StringPrintf("/Caster%d.txt", _index);
	_myFiles.WriteFile(fileName.c_str(), llText.c_str());

	LoadSettings(); // Reload the settings after saving

	_forceReconnect = true; // Force a reconnect to use the new settings
}

///////////////////////////////////////////////////////////////////////////////
// Loop here forever in a separate thread
// This is the main loop for the RTK connection
void NTRIPServer::TaskFunction()
{
	Serial.printf("+++++ NTRIP Server %d Starting\r\n", _index);
	while (true)
	{
		// Get the next item from the queue
		QueueData *pItem = DequeueData();
		if (pItem == nullptr)
		{
			// No data to send so wait 2ms before trying again
			vTaskDelay(2 / portTICK_PERIOD_MS);
			continue;
		}

		// Every 10 seconds log stack height and task stack height
		if ((millis() - _lastStackCheck) > 10000)
		{
			_lastStackCheck = millis();
			_maxStackHeight = uxTaskGetStackHighWaterMark(NULL);
			Serial.printf("%d) Stack %d\r\n", _index, _maxStackHeight);
		}

		// Wifi check interval
		if (_client.connected())
		{
			ConnectedProcessing(pItem->getData(), pItem->getLength());
		}
		else
		{
			vTaskDelay(200 / portTICK_PERIOD_MS);
			_wasConnected = false;
			_status = ConnectionState::Disconnected;
			Reconnect();
		}

		// Delete the item
		delete pItem;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Process the data when connected
void NTRIPServer::ConnectedProcessing(const byte *pBytes, int length)
{
	if (!_wasConnected)
	{
		_reconnects++;
		_status = ConnectionState::Connected;
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
	// Skip if we have no data
	if (length < 1)
		return;

	// Check for a forced reconnect
	if (_forceReconnect)
	{
		_forceReconnect = false;
		LogX(StringPrintf("RTK %s Reconnecting due to forced reconnect", _sAddress.c_str()));
		_client.stop();
		_status = ConnectionState::Disconnected;
		return;
	}

	// Send and record time
	unsigned long startT = micros();
	int sent = _client.write(pBytes, length);

	// Record the time delay and max write time
	unsigned long time = micros() - startT;

	if (sent != length)
	{
		// Send failed so record the failure and start the reconnect process
		LogX(StringPrintf("E500 - %s Only sent %d of %d (%dms)",
						  _sAddress.c_str(),
						  sent,
						  length,
						  time / 1000));

		int errorCode = errno;
		const char *errorMsg = strerror(errno);
		// LogX(StringPrintf(" --- Error: %d - %s", errorCode, errorMsg));

		if (errorCode == EWOULDBLOCK)
		{
			LogX(StringPrintf(" --- Socket would block - buffer full. Try %d", _consecutiveTimeouts));
			_totalTimeouts++;
			vTaskDelay(100 / portTICK_PERIOD_MS);
			_consecutiveTimeouts++;
			if (_consecutiveTimeouts < 3)
				return;
		}
		_consecutiveTimeouts = 0;

		// Check specific error conditions
		if (errorCode == ENOTCONN)
			LogX(" --- Socket not connected");
		else if (errorCode == EWOULDBLOCK)
			LogX(" --- Socket would block - buffer full");
		else if (errorCode == ECONNRESET)
			LogX(" --- Connection reset by peer");
		else if (errorCode == ETIMEDOUT)
			LogX(" --- Connection timed out");
		else if (errorCode == ENOTCONN)
			LogX(" --- Socket not connected");
		else if (errorCode == EPIPE)
			LogX(" --- Broken pipe - connection closed by peer");
		else if (errorCode == EINVAL)
			LogX(" --- Invalid argument - check socket options");
		else
			LogX(StringPrintf(" --- Error: %d - %s", errorCode, errorMsg));

		_client.stop();
		_status = ConnectionState::Disconnected;
	}
	else
	{
		// Good send so clear the timeout count and record the time
		_consecutiveTimeouts = 0;
		_wifiConnectTime = millis();
		_packetsSent++;
		_timeOutIndex = 0;

		// Record max send time
		if (_maxSendTime == 0)
			_maxSendTime = time;
		else
			_maxSendTime = max(_maxSendTime, time);

		// Logf("RTK %s Sent %d OK", _sAddress.c_str(), sent);
		//_sendMicroSeconds.push_back(sent * 8 * 1000 / max(1UL, time));
		_history.AddNtripSendTime(_index, (int)time);
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
	LogX("RECV. " + _sAddress + "\r\n" + HexAsciDump(_pSocketBuffer, buffSize));
}

///////////////////////////////////////////////////////////////////////////////
// Write to the debug log and keep the last few messages for display
void NTRIPServer::LogX(std::string text)
{
	auto s = Uptime(millis()) + " " + text;
	if (xSemaphoreTake(_logMutex, portMAX_DELAY))
	{
		_logHistory.push_back(s);
		TruncateLog(_logHistory);
		xSemaphoreGive(_logMutex);
	}
	else
	{
		perror("LogX:Failed to take log mutex\n");
	}
}

////////////////////////////////////////////////////////////////////////////
// Get a copy of the log safely
std::vector<std::string> NTRIPServer::GetLogHistory()
{
	std::vector<std::string> copyVector;
	if (xSemaphoreTake(_logMutex, portMAX_DELAY))
	{
		copyVector.insert(copyVector.end(), _logHistory.begin(), _logHistory.end());
		xSemaphoreGive(_logMutex);
	}
	else
	{
		perror("CopyMainLog:Failed to take log mutex\n");
	}
	return copyVector;
}

////////////////////////////////////////////////////////////////////////////////
bool NTRIPServer::Reconnect()
{
	// Limit how soon the connection is retried
	if ((millis() - _wifiConnectTime) < WIFI_TIMEOUTS[_timeOutIndex])
		return false;

	// Get next timeout period
	_timeOutIndex++;
	if (_timeOutIndex >= WIFI_TIMEOUT_SIZE)
		_timeOutIndex = WIFI_TIMEOUT_SIZE - 1;

	_wifiConnectTime = millis();

	// Start the connection process
	LogX(StringPrintf("RTK Connecting to %s : %d", _sAddress.c_str(), _port));

	int status = _client.connect(_sAddress.c_str(), _port);
	LogX(StringPrintf("RTK %s Connect status %d", _sAddress.c_str(), status));

	// [ 30824][E][WiFiClient.cpp:320] setSocketOption(): fail on -1, errno: 9, "Bad file number"
	_client.setNoDelay(true); // This results in 0.5s latency when RTK2GO.com is skipped?
	if (!_client.connected())
	{
		LogX(StringPrintf("E500 - RTK %s Not connected %d. (%dms)", _sAddress.c_str(), status, millis() - _wifiConnectTime));
		return false;
	}
	auto s = StringPrintf("Connected %s OK. (%dms)", _sAddress.c_str(), millis() - _wifiConnectTime);
	LogX(s);
	Logln(s.c_str());

	if (!WriteText(StringPrintf("SOURCE %s %s\r\n", _sPassword.c_str(), _sCredential.c_str()).c_str()))
		return false;
	if (!WriteText("Source-Agent: NTRIP UM98/ESP32_T_Display_S3\r\n"))
		return false;
	if (!WriteText("STR: \r\n"))
		return false;
	if (!WriteText("\r\n"))
		return false;
	return true;
}

bool NTRIPServer::WriteText(const char *str)
{
	if (str == NULL)
		return true;

	std::string message = StringPrintf("    -> '%s'", str);
	ReplaceCrLfEncode(message);
	LogX(message);

	size_t len = strlen(str);
	size_t written = _client.write((const uint8_t *)str, len);
	if (len == written)
		return true;

	// Failed to write
	LogX(StringPrintf("Write failed %s", _sAddress.c_str()));
	return false;
}

////////////////////////////////////////////////////////////////////////////////
// Get the connection status as a string
const char *NTRIPServer::GetStatus() const
{
	switch (_status)
	{
	case ConnectionState::Connected:
		return "Connected";
	case ConnectionState::Disconnected:
		return "Disconnected";
	case ConnectionState::Disabled:
		return "Disabled";
	default:
		return "Unknown";
	}
}

///////////////////////////////////////////////////////////////////////////////
// Add an item to the queue
// If memory allocation for QueueData fails, the method returns false.
bool NTRIPServer::EnqueueData(const byte *pBytes, int length)
{
	// Don't queue if disabled
	if (_status == ConnectionState::Disabled)
		return false;

	// Lock the queue mutex
	if (xSemaphoreTake(_queMutex, portMAX_DELAY))
	{
		bool overflowed = false;
		while (_dataQueue.size() > 20)
		{
			overflowed = true;
			_overflowSetSize++;
			// Don't count dumps when not connected
			// if (_status != ConnectionState::Connected)
			_queueOverflows++;
			QueueData *pOldItem = _dataQueue[0];
			// LogX(StringPrintf("Queue %d overflow %d bytes", _index, pOldItem->getLength()));
			_dataQueue.erase(_dataQueue.begin());
			delete pOldItem;
		}

		// Log the number of overflows
		if (!overflowed && _overflowSetSize > 0)
		{
			LogX(StringPrintf("Queue %d overflow %d", _index, _overflowSetSize));
			_overflowSetSize = 0;
		}

		// Create queue item
		QueueData *pItem = new (std::nothrow) QueueData(pBytes, length);
		if (pItem == nullptr)
		{
			// Memory allocation failed
			LogX("Failed to allocate memory for QueueData");
			xSemaphoreGive(_queMutex);
			return false;
		}
		_dataQueue.push_back(pItem);

		xSemaphoreGive(_queMutex);
		return true;
	}
	else
	{
		LogX("Failed to take queue mutex");
		return false;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Get the next item from the queue
// Returns nullptr if the queue is empty.
// WARNING : The caller is responsible for deleting the returned item.
QueueData *NTRIPServer::DequeueData()
{
	if (xSemaphoreTake(_queMutex, portMAX_DELAY))
	{
		while (_dataQueue.size() > 0)
		{
			QueueData *pItem = _dataQueue[0];
			_dataQueue.erase(_dataQueue.begin());

			// Check for null item (This should never happen)
			if (pItem == nullptr)
			{
				LogX("DequeueData: Null item in queue");
				continue;
			}

			// Check for expired items
			if (pItem->IsExpired(500))
			{
				_expiredPackets++;
				delete pItem;
				continue;
			}

			// Return the item
			xSemaphoreGive(_queMutex);
			return pItem;
		}

		// Queue is empty
		xSemaphoreGive(_queMutex);
	}
	return nullptr;
}