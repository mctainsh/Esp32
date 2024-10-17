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
		Logln("SPIFFS Mount Failed");
		return false;
	}


	bool WriteFile(const char* path, const char* message)
	{
		Logf("Writing file: %s -> '%s'\r\n\t", path, message);
		fs::File file = SPIFFS.open(path, FILE_WRITE);
		if (!file)
		{
			Logln("- failed to open file for writing");
			return false;
		}

		bool error = file.print(message);
		Logln(error ? "- file written" : "- write failed");

		file.close();
		return error;
	}


	void AppendFile(const char* path, const char* message)
	{
		Logf("Appending to file: %s -> '%s'\r\n\t", path, message);
		fs::File file = SPIFFS.open(path, FILE_APPEND);
		if (!file)
		{
			Logln("- failed to open file for appending");
			return;
		}
		if (file.print(message))
		{
			Logln("- message appended");
		}
		else
		{
			Logln("- append failed");
		}
		file.close();
	}


	bool ReadFile(const char* path, std::string& text)
	{
		Logf("Reading file: %s\r\n\t", path);

		fs::File file = SPIFFS.open(path);
		if (!file || file.isDirectory())
		{
			Logln("- failed to open file for reading");
			return false;
		}
		Logln("- read from file:");
		while (file.available())
			text += file.read();
		file.close();
		return true;
	}
  private:
};