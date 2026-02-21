#ifndef VOXEL_ENGINE_LOG_H
#define VOXEL_ENGINE_LOG_H

#include <string>

class Log {
private:
    static bool debugDisabled;
    static bool infoDisabled;
    static bool warningDisabled;
    static bool errorDisabled;

    static constexpr char const *TYPE_PREFIX = "  | ";
    static constexpr char const *TYPE_SUFFIX = " |  ";
    static constexpr char const *DEBUG_COLOR = "\033[37m";
    static constexpr char const *INFO_COLOR = "\033[97m";
    static constexpr char const *WARNING_COLOR = "\033[93m";
    static constexpr char const *ERROR_COLOR = "\033[91m";
    static constexpr char const *RESET_COLOR = "\033[0m";

    static std::string GetTime();

public:
    Log() = delete;

    static void Debug(const char *message);
    static void Info(const char *message);
    static void Warning(const char *message);
    static void Error(const char *message);

    static void SetDebugEnabled(bool enabled);
    static void SetInfoEnabled(bool enabled);
    static void SetWarningEnabled(bool enabled);
    static void SetErrorEnabled(bool enabled);
};

#endif //VOXEL_ENGINE_LOG_H
