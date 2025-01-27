#pragma once

#include <string>
#include <vector>

//void Log(const char *msg);
std::string Logln(const char *msg);
std::string AddToLog(const char *msg);
std::string Uptime(unsigned long millis);
void TruncateLog( std::vector<std::string> &log );

template<typename... Args>
void Logf(const std::string& format, Args... args);

#include "HandyLog.tpp"
