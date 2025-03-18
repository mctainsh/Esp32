#pragma once

#include <string>
#include <vector>

void SetupLog();
std::string Logln(const char *msg);

template<typename... Args>
void Logf(const std::string& format, Args... args);

const std::string Uptime(unsigned long millis);
const void TruncateLog( std::vector<std::string> &log );
const std::vector<std::string> CopyMainLog();

#include "HandyLog.tpp"
