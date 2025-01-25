#include "HandyLog.h"
#include <HandyString.h>
#include <Global.h>

std::vector<std::string> _mainLog;

std::string Uptime(unsigned long millis)
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

std::string Logln(const char *msg)
{
	auto s = AddToLog(msg);
	Serial.print(s.c_str());
	Serial.print("\r\n");
	return s;
}

std::string AddToLog(const char *msg)
{
	std::string s = StringPrintf("%s %s", Uptime(millis()).c_str(), msg);

	if (_mainLog.capacity() < MAX_LOG_LENGTH)
		_mainLog.reserve(MAX_LOG_LENGTH);

	// Remove the oldest while too long
	while (_mainLog.size() > MAX_LOG_LENGTH)
		_mainLog.erase(_mainLog.begin());

	// Add everything except the last CR and LF
	

	_mainLog.push_back(s);
	return s;
}
