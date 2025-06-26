#pragma once

#include "FS.h"
#include "SPIFFS.h"
#include "HandyLog.h"

/* You only need to format SPIFFS the first time you run a
   test or else use the SPIFFS plugin to create a partition
   https://github.com/me-no-dev/arduino-esp32fs-plugin */
#define FORMAT_SPIFFS_IF_FAILED true

#define BOOT_LOG_FILENAME "/bootlog.txt"
#define BOOT_LOG_MAX_LENGTH 100
#define LOG_FILE_PREFIX "/logs/Log_"

///////////////////////////////////////////////////////////////////////////////
// File access routines
class MyFiles
{
	int _logLength = -1;		 // Length of the log file
	fs::File _fsLog;			 // Log file
	SemaphoreHandle_t _mutexLog; // Thread safe access to writing logs
	SemaphoreHandle_t _mutex;	 // Thread safe access
public:
	bool Setup()
	{
		// Setup mutex
		_mutex = xSemaphoreCreateMutex();
		_mutexLog = xSemaphoreCreateMutex();
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

	void StartLogFile()
	{
		_logLength = -1; // Reset the log length
		_fsLog = SPIFFS.open((LOG_FILE_PREFIX + _handyTime.FileSafe() + ".log").c_str(), FILE_WRITE);
		if (_fsLog)
			_logLength = 0;
	}

	////////////////////////////////////////////////////////////////////////////////
	/// @brief Get a sorted list of all files in the SPIFFS file system.
	/// @return A vector of fs::File objects sorted by their path.
	std::vector<fs::File> getAllFilesSorted()
	{
		std::vector<fs::File> files;
		auto root = SPIFFS.open("/");
		auto file = root.openNextFile();
		while (file)
		{
			files.push_back(file);
			file = root.openNextFile();
		}
		std::sort(files.begin(), files.end(), [](const fs::File &a, const fs::File &b) {
			return strcmp(a.path(), b.path()) < 0;
		});
		return files;
	}

	////////////////////////////////////////////////////////////////////////////////
	/// @brief Called when the SPIFFS file system is started to list drive contents
	void StartupComplete()
	{
		// Dump the file system contents
		Logln("Drive contents");
		auto files = getAllFilesSorted();
		if (files.empty())
		{
			Logln("\tNo files found in SPIFFS");
		}
		else
		{
			for (const auto &file : files)
			{
				Logf("\t %9d - %s", file.size(), file.path());
			}
		}

		// auto root = SPIFFS.open("/");
		// auto file = root.openNextFile();
		// std::string fileNames = "";
		// while (file)
		// {
		// 	fileNames += StringPrintf("\t %9d - %s\n", file.size(), file.path());
		// 	//Logf("\t %9d - %s", file.size(), file.path());
		// 	file = root.openNextFile();
		// }
		// Logf(fileNames.c_str());

		// Check the usage
		size_t totalBytes = SPIFFS.totalBytes();
		size_t usedBytes = SPIFFS.usedBytes();
		size_t freeBytes = totalBytes - usedBytes;
		Logf("\tUsed :%8u bytes", usedBytes);
		Logf("\tFree :%8u bytes", freeBytes);
		Logf("\tTotal:%8u bytes", totalBytes);

		// Record this startup
		auto file = SPIFFS.open(BOOT_LOG_FILENAME, FILE_APPEND);
		if (file)
		{
			file.printf("%s - %s\n", _handyTime.LongString().c_str(), APP_VERSION);
			file.close();
		}
		else
		{
			Logf("E207 - Failed to %s file for appending", BOOT_LOG_FILENAME);
			return;
		}

		// Dump the history of startups
		auto logFile = SPIFFS.open(BOOT_LOG_FILENAME, FILE_READ);
		String log = "";
		int count = 0;
		while (logFile.available())
		{
			String line = logFile.readStringUntil('\n');
			if (line.length())
			{
				Serial.printf("\t%3d: %s\n", count, line.c_str());
				count++;
			}
		}
		logFile.close();
	}

	////////////////////////////////////////////////////////////////////////////////
	/// @brief Write a message to the file system.
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
				if (ch == '\0')
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

	////////////////////////////////////////////////////////////////////////////////
	/// @brief Load the string from the file system.
	/// @param readText Value to populate with the read text.
	/// @param path Location of the file to read.
	/// @note If the file does not exist, readText will not be set
	/// @param maxLength Maximum length of the string to read.
	void LoadString(std::string &readText, const char *path, int maxLength = 256)
	{
		std::string text;
		if (ReadFile(path, text))
		{
			Logln(StringPrintf(" - Read config '%s'", text.c_str()).c_str());
			readText.assign(text);
		}
		else
		{
			Logln(StringPrintf(" - E742 - Cannot read saved Server setting '%s'", path).c_str());
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	/// @brief Save the string to the file system.
	void AppendLog(const char *message)
	{
		if (_logLength < 0)
			return;

		_logLength += strlen(message);
		if (xSemaphoreTake(_mutexLog, portMAX_DELAY))
		{
			if (_logLength > 100000)
			{
				if (_fsLog)
					_fsLog.close();
				StartLogFile();
			}
			if (_fsLog)
			{
				_fsLog.println(message);
				_fsLog.flush();
			}
			xSemaphoreGive(_mutexLog);
		}
	}
};