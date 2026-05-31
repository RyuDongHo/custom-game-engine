#include "Logger.h"

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <utility>

#include "AuthorMap.h"

namespace {
std::string CurrentTimeString()
{
    std::time_t now = std::time(nullptr);
    std::tm localTime = {};
    localtime_s(&localTime, &now);

    char buffer[32] = {};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &localTime);
    return buffer;
}

const char* LevelToString(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug: return "[DEBUG]";
    case LogLevel::Info: return "[INFO ]";
    case LogLevel::Warning: return "[WARN ]";
    case LogLevel::Error: return "[ERROR]";
    default: return "[UNKWN]";
    }
}

class ConsoleLogSink : public ILogSink
{
public:
    void Write(const LogEntry& entry) override
    {
        std::printf("%s %s [%s] %s\n",
            CurrentTimeString().c_str(),
            LevelToString(entry.level),
            entry.author ? entry.author : "?",
            entry.message.c_str());
        std::fflush(stdout);
    }
};
}

Logger& Logger::Get()
{
    static Logger instance;
    return instance;
}

Logger::Logger()
{
    AddSink(std::make_unique<ConsoleLogSink>());
}

void Logger::Debug(const char* file, const char* format, ...)
{
    va_list args; va_start(args, format);
    WriteFormatted(LogLevel::Debug, file, format, args);
    va_end(args);
}

void Logger::Info(const char* file, const char* format, ...)
{
    va_list args; va_start(args, format);
    WriteFormatted(LogLevel::Info, file, format, args);
    va_end(args);
}

void Logger::Warning(const char* file, const char* format, ...)
{
    va_list args; va_start(args, format);
    WriteFormatted(LogLevel::Warning, file, format, args);
    va_end(args);
}

void Logger::Error(const char* file, const char* format, ...)
{
    va_list args; va_start(args, format);
    WriteFormatted(LogLevel::Error, file, format, args);
    va_end(args);
}

void Logger::AddSink(std::unique_ptr<ILogSink> sink)
{
    if (!sink) return;
    std::lock_guard<std::mutex> lock(mutex);
    sinks.push_back(std::move(sink));
}

void Logger::ClearSinks()
{
    std::lock_guard<std::mutex> lock(mutex);
    sinks.clear();
}

void Logger::Write(const LogEntry& entry)
{
    std::lock_guard<std::mutex> lock(mutex);
    for (const auto& sink : sinks) {
        sink->Write(entry);
    }
}

void Logger::WriteFormatted(LogLevel level, const char* file, const char* format, va_list args)
{
    if (format == nullptr) return;

    char buffer[1024] = {};
    vsnprintf(buffer, sizeof(buffer), format, args);

    LogEntry entry;
    entry.level   = level;
    entry.file    = file;
    entry.author  = AuthorMap::Lookup(file);
    entry.message = buffer;
    Get().Write(entry);
}
