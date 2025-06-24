#pragma once

#include "FS.h"
#include "SPIFFS.h"
#include "HandyLog.h"

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
		// Setup mutex
		_mutex = xSemaphoreCreateMutex();
		if (_mutex == NULL)
			perror("Failed to create FILE mutex\n");
		else
			Serial.printf("File Mutex Created\r\n", index);

		// Check if the file system is mounted
		if (SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
			return true;
		Logln("SPIFFS Mount Failed");
		return false;
	}

	bool WriteFile(const char *path, const char *message)
	{
		Logf("Writing file: %s -> '%s'", path, message);
		bool error = true;
		if (xSemaphoreTake(_mutex, portMAX_DELAY))
		{
			fs::File file = SPIFFS.open(path, FILE_WRITE);
			if (!file)
			{
				Logln("- failed to open file for writing");
				xSemaphoreGive(_mutex);
				return false;
			}

			error = file.print(message);
			Logln(error ? "- file written" : "- write failed");

			file.close();
			xSemaphoreGive(_mutex);
		}
		return error;
	}

	void AppendFile(const char *path, const char *message)
	{
		Logf("Appending to file: %s -> '%s'", path, message);
		if (xSemaphoreTake(_mutex, portMAX_DELAY))
		{
			fs::File file = SPIFFS.open(path, FILE_APPEND);
			if (!file)
			{
				Logln("- failed to open file for appending");
				xSemaphoreGive(_mutex);
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
			xSemaphoreGive(_mutex);
		}
	}

	bool ReadFile(const char *path, std::string &text, int maxLength = 256)
	{
		Logf("Reading file: %s", path);
		if (xSemaphoreTake(_mutex, portMAX_DELAY))
		{
			fs::File file = SPIFFS.open(path);
			if (!file || file.isDirectory())
			{
				Logln("- failed to open file for reading");
				xSemaphoreGive(_mutex);
				return false;
			}
			Serial.println("- read from file:");
			while (file.available())
			{
				char ch = static_cast<char>(file.read());
				if( ch == '\0')
				{
					Logln("- read NULL character");
					break;
				}
				text += ch;
				if (text.length() > maxLength) 
				{
					Logf("- read %d bytes is greater than ", text.length(), maxLength);
					break;
				}
			}
			file.close();
			xSemaphoreGive(_mutex);
		}
		return true;
	}

private:
	SemaphoreHandle_t _mutex; // Thread safe access
};