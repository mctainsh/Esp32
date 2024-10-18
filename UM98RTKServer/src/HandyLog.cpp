#include "HandyLog.h"


void Log(const char *msg)
{
	Serial.printf("%05d %s", millis(), msg);
}


void Logln(const char *msg)
{
	Serial.printf("%05d %s\r\n", millis(), msg);
}


