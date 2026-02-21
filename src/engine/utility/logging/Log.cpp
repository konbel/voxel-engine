#include "Log.h"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <ostream>

bool Log::debugDisabled = false;
bool Log::infoDisabled = false;
bool Log::warningDisabled = false;
bool Log::errorDisabled = false;

////////////////////////////////////////////////////////////////////////////////
std::string Log::GetTime() {
    const time_t now = std::time(nullptr);
    std::stringstream buffer;
    buffer << "[" << std::put_time(localtime(&now), "%d-%m-%Y %H:%M:%S") << "]";
    return buffer.str();
}

////////////////////////////////////////////////////////////////////////////////
void Log::Debug(const char *message) {
    if (debugDisabled) {
        return;
    }

    std::cout << GetTime() << TYPE_PREFIX << DEBUG_COLOR << "DEBUG  " << RESET_COLOR << TYPE_SUFFIX << message << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
void Log::Info(const char *message) {
    if (infoDisabled) {
        return;
    }

    std::cout << GetTime() << TYPE_PREFIX << INFO_COLOR << "INFO   " << RESET_COLOR << TYPE_SUFFIX <<  message << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
void Log::Warning(const char *message) {
    if (warningDisabled) {
        return;
    }

    std::cout << GetTime() << TYPE_PREFIX << WARNING_COLOR << "WARNING" <<  RESET_COLOR << TYPE_SUFFIX << message << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
void Log::Error(const char *message) {
    if (errorDisabled) {
        return;
    }

    std::cout << GetTime() << TYPE_PREFIX << ERROR_COLOR << "ERROR  " << RESET_COLOR << TYPE_SUFFIX << message << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
void Log::SetDebugEnabled(const bool enabled) {
    debugDisabled = !enabled;
}

////////////////////////////////////////////////////////////////////////////////
void Log::SetInfoEnabled(const bool enabled) {
    infoDisabled = !enabled;
}

////////////////////////////////////////////////////////////////////////////////
void Log::SetWarningEnabled(const bool enabled) {
    warningDisabled = !enabled;
}

////////////////////////////////////////////////////////////////////////////////
void Log::SetErrorEnabled(const bool enabled) {
    errorDisabled = !enabled;
}
