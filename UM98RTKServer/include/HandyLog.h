#pragma once

#include <string>

//void Log(const char *msg);
void Logln(const char *msg);
std::string AddToLog(const char *msg);
std::string Uptime(unsigned long millis);

template<typename... Args>
void Logf(const std::string& format, Args... args);

#include "HandyLog.tpp"
