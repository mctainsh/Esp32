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
#define LOG_FILE_PREFIX "/logs/"

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

	////////////////////////////////////////////////////////////////////////////////
	/// @brief Start a new logging file or open the existing one
	void StartLogFile(std::vector<std::string> *pMainLog)
	{
		if (!xSemaphoreTake(_mutexLog, portMAX_DELAY))
			return;

		_logLength = -1; // Reset the log length
		// Open the current file if already exists
		if (_fsLog)
		{
			_fsLog.println("**** CLOSING EXISTING FILE FOR ROLLOVER ****");
			_fsLog.close(); // Close the existing log file
		}

		// Remove old log files if too much drive is used
		Serial.print("DUMP Files\n");
		auto files = GetAllFilesSorted();

		// Remove non-log files from the list
		for (int i = files.size()-1; i >= 0; i--)
			if(files[i].Path.find(LOG_FILE_PREFIX) != 0 || files[i].IsCurrentLog)
				files.erase(files.begin() + i);

		// Starting at the first item, remove files until we have less than 250kb available
		size_t totalFree = SPIFFS.totalBytes() - SPIFFS.usedBytes();
		for (const auto &file : files)
		{
			if( totalFree > 250*1000)
				break;
			
			totalFree += file.Size;
			SPIFFS.remove(file.Path.c_str());
		}

		// Display the file we are working with
		for (const auto &file : files)
			Serial.printf("Log file: %s (%d bytes)\r\n", file.Path.c_str(), file.Size);

		// Open existing or create a new log file
		auto filename = LOG_FILE_PREFIX + _handyTime.FileSafe() + ".txt";
		if (SPIFFS.exists(filename.c_str()))
		{
			_fsLog = SPIFFS.open(filename.c_str(), FILE_APPEND);
			_fsLog.println("**** APPENDING EXISTING FILE ****");
			_logLength = _fsLog.size(); // Get the size of the existing log file
		}
		if (!_fsLog)
		{
			_fsLog = SPIFFS.open(filename.c_str(), FILE_WRITE);
			if (_fsLog)
				_logLength = 0;
		}
		xSemaphoreGive(_mutexLog);

		// Copy the main log into the log file
		if (pMainLog != NULL)
			for (const auto &line : *pMainLog)
				AppendLog(line.c_str());
	}

	////////////////////////////////////////////////////////////////////////////////
	/// @brief Details of a log file
	struct LogFileSummary
	{
		std::string Path;
		int Size;
		bool IsCurrentLog = false; // Indicate if this is the current log file

		LogFileSummary(fs::File f, bool isCurrentLog) : Path(f.path()), Size(f.size()), IsCurrentLog(isCurrentLog) {}

		bool operator<(const LogFileSummary &other) const
		{
			return strcmp(Path.c_str(), other.Path.c_str()) < 0;
		}
	};

	////////////////////////////////////////////////////////////////////////////////
	/// @brief Get a sorted list of all files in the SPIFFS file system.
	/// @return A vector of fs::File objects sorted by their path.
	std::vector<LogFileSummary> GetAllFilesSorted()
	{
		const char *logPath = _fsLog ? _fsLog.path() : "";
		std::vector<LogFileSummary> files;
		auto root = SPIFFS.open("/");
		auto file = root.openNextFile();
		while (file)
		{
			// Note. If a file is modified during this stage, it will be duplicated in the list

			auto it = std::find_if(files.begin(), files.end(),
								   [&](const LogFileSummary &log)
								   { return strcmp(log.Path.c_str(), file.path()) == 0; });
			if (it == files.end()) // Only add if not already in the list
			{
				files.push_back(LogFileSummary(file, strcmp(file.path(), logPath) == 0));
			}
			file = root.openNextFile();
		}

		// Sort the list of files by their path
		std::sort(files.begin(), files.end(), [](const LogFileSummary &a, const LogFileSummary &b)
				  { return strcmp(a.Path.c_str(), b.Path.c_str()) < 0; });
		return files;
	}

	////////////////////////////////////////////////////////////////////////////////
	/// @brief Called when the SPIFFS file system is started to list drive contents
	void StartupComplete()
	{
		// Dump the file system contents
		Logln("Drive contents");
		auto files = GetAllFilesSorted();
		if (files.empty())
		{
			Logln("\tNo files found in SPIFFS");
		}
		else
		{
			for (const auto &file : files)
			{
				Logf("\t %9d - %s", file.Size, file.Path.c_str());
			}
		}

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

		if (_logLength > 100000)
		{
			std::vector<std::string> logHeader;
			logHeader.push_back(StringPrintf("***** Rolling over from log file, length %s", _fsLog.path()));
			StartLogFile(&logHeader);
		}

		if (!xSemaphoreTake(_mutexLog, portMAX_DELAY))
			return;
		if (_fsLog)
		{
			_fsLog.println(message);
			_fsLog.flush();
		}
		xSemaphoreGive(_mutexLog);
	}

	////////////////////////////////////////////////////////////////////////////////
	/// @brief Close the log file if it is open.
	void CloseLogFile()
	{
		if (_fsLog)
		{
			_fsLog.close();
			_logLength = -1; // Reset the log length
		}
	}
};