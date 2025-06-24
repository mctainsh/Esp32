#include "HandyLog.h"
#include <HandyString.h>
#include <Global.h>
// #include "freertos/semphr.h"

std::string AddToLog(const char *msg, bool timePrefix = true);

std::vector<std::string> _mainLog;

static SemaphoreHandle_t _serialMutex;

//////////////////////////////////////////////////////////////////////////
// Setup the logging stuff
void SetupLog()
{
	_serialMutex = xSemaphoreCreateMutex();
	if (_serialMutex == NULL)
		perror("Failed to create serial mutex\n");
	else
		Logln("Serial Mutex Created");
}

////////////////////////////////////////////////////////////////////////////
// Get a copy of the main log safely
const std::vector<std::string> CopyMainLog()
{
	std::vector<std::string> copyVector;
	if (xSemaphoreTake(_serialMutex, portMAX_DELAY))
	{
		copyVector.insert(copyVector.end(), _mainLog.begin(), _mainLog.end());
		xSemaphoreGive(_serialMutex);
	}
	else
	{
		perror("CopyMainLog:Failed to take serial mutex\n");
	}
	return copyVector;
}

const std::string Uptime(unsigned long millis)
{
	uint32_t t = millis / 1000;
	std::string uptime = StringPrintf(":%02d.%03d", t % 60, millis % 1000);
	t /= 60;
	uptime = StringPrintf(":%02d", t % 60) + uptime;
	t /= 60;
	uptime = StringPrintf("%02d", t % 24) + uptime;
	t /= 24;
	uptime = StringPrintf("%d ", t) + uptime;
	return uptime;
}

std::string Logln(const char *msg, bool timePrefix)
{
	std::string s = AddToLog(msg, timePrefix);
#ifdef SERIAL_LOG
	// perror(s.c_str());
	Serial.print(s.c_str());
	Serial.print("\r\n");
#endif
	return s;
}

////////////////////////////////////////////////////////////////////////////////////////
/// @brief Trim the log to the size limit
const void TruncateLog(std::vector<std::string> &log)
{
	// Truncate based on total log size
	while (log.size() > MAX_LOG_LENGTH)
		log.erase(log.begin());

	// Truncate based on total size
	while (true)
	{
		int total = 0;
		for (std::string line : log)
			total += line.length();
		if (total < MAX_LOG_SIZE)
			break;
		log.erase(log.begin());
	}
}

std::string AddToLog(const char *msg, bool timePrefix)
{
	std::string s;
	std::string time = timePrefix ? _handyTime.LongString() : "\t\t";
	if (xSemaphoreTake(_serialMutex, portMAX_DELAY))
	{
		//s = StringPrintf("%s %s", Uptime(millis()).c_str(), msg);
		s = StringPrintf("%s %s", time.c_str(), msg);

		if (_mainLog.capacity() < MAX_LOG_LENGTH)
			_mainLog.reserve(MAX_LOG_LENGTH);

		// Remove the oldest while too long
		while (_mainLog.size() > MAX_LOG_LENGTH)
			_mainLog.erase(_mainLog.begin());

		// Add the first 127 characters of the message
		if (s.length() > MAX_LOG_ROW_LENGTH)
			_mainLog.push_back(s.substr(0, MAX_LOG_ROW_LENGTH) + "...");
		else
			_mainLog.push_back(s);

		xSemaphoreGive(_serialMutex);
	}
	else
	{
		perror("AddToLog:Failed to take serial mutex\n");
	}
	return s;
}
