#include "HandyLog.h"

#include <string>
#include <HardwareSerial.h>
#include <memory>

///////////////////////////////////////////////////////////////////////////////
// Taken from
//	https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
// Answer by iFreiticht
template<typename... Args>
void Logf(const std::string& format, Args... args)
{
	int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;  // Extra space for '\0'
	if (size_s <= 0)
	{
		Serial.print("E910 - Unable to allocate string size. ");
		Logln(format.c_str());
		return;
	}
	auto size = static_cast<size_t>(size_s);
	std::unique_ptr<char[]> buf(new char[size]);
	std::snprintf(buf.get(), size, format.c_str(), args...);
	Log(std::string(buf.get(), buf.get() + size - 1).c_str());  // We don't want the '\0' inside
}

