#pragma once

#include "FS.h"
#include "SPIFFS.h"
/* You only need to format SPIFFS the first time you run a
   test or else use the SPIFFS plugin to create a partition
   https://github.com/me-no-dev/arduino-esp32fs-plugin */
#define FORMAT_SPIFFS_IF_FAILED true

///////////////////////////////////////////////////////////////////////////////
// File access routines
class MyFiles
{
  public:

	bool Setup()
	{
		if (SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
			return true;
		Serial.println("SPIFFS Mount Failed");
		return false;
	}


	bool WriteFile(const char* path, const char* message)
	{
		Serial.printf("Writing file: %s -> '%s'\r\n\t", path, message);
		fs::File file = SPIFFS.open(path, FILE_WRITE);
		if (!file)
		{
			Serial.println("- failed to open file for writing");
			return false;
		}

		bool error = file.print(message);
		Serial.println(error ? "- file written" : "- write failed");

		file.close();
		return error;
	}


	void AppendFile(const char* path, const char* message)
	{
		Serial.printf("Appending to file: %s -> '%s'\r\n\t", path, message);
		fs::File file = SPIFFS.open(path, FILE_APPEND);
		if (!file)
		{
			Serial.println("- failed to open file for appending");
			return;
		}
		if (file.print(message))
		{
			Serial.println("- message appended");
		}
		else
		{
			Serial.println("- append failed");
		}
		file.close();
	}


	bool ReadFile(const char* path, std::string& text)
	{
		Serial.printf("Reading file: %s\r\n\t", path);

		fs::File file = SPIFFS.open(path);
		if (!file || file.isDirectory())
		{
			Serial.println("- failed to open file for reading");
			return false;
		}
		Serial.println("- read from file:");
		while (file.available())
			text += file.read();
		file.close();
		return true;
	}
  private:
};