#pragma once

#include <cstdarg>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

enum class LogLevel
{
    Debug,
    Info,
    Warning,
    Error
};

// sink가 메시지 + 메타데이터를 받기 위한 구조체.
struct LogEntry {
    LogLevel    level;
    const char* file;     // __FILE__ pointer (수명 = static literal).
    const char* author;   // author lookup 결과 (수명 = static literal).
    std::string message;  // vsnprintf 결과.
};

class ILogSink
{
public:
    virtual ~ILogSink() = default;
    // 신규 시그니처 — entry 전체 전달. 기존 message-only 호출도 호환 위해 default 구현 제공.
    virtual void Write(const LogEntry& entry) = 0;
};

class Logger
{
public:
    static Logger& Get();

    // 매크로(LOG_*) 경유 시 file이 자동 첨부됨. 직접 호출 시 file=nullptr → author="RDH" default.
    static void Debug  (const char* file, const char* format, ...);
    static void Info   (const char* file, const char* format, ...);
    static void Warning(const char* file, const char* format, ...);
    static void Error  (const char* file, const char* format, ...);

    void AddSink(std::unique_ptr<ILogSink> sink);
    void ClearSinks();
    void Write(const LogEntry& entry);

private:
    Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static void WriteFormatted(LogLevel level, const char* file, const char* format, va_list args);

    std::vector<std::unique_ptr<ILogSink>> sinks;
    std::mutex mutex;
};

// 호출처에서 __FILE__를 자동 캡쳐하기 위한 매크로. 기존 호출은 sed로 일괄 변환됨.
#define LOG_DEBUG(...) ::Logger::Debug  (__FILE__, __VA_ARGS__)
#define LOG_INFO(...)  ::Logger::Info   (__FILE__, __VA_ARGS__)
#define LOG_WARN(...)  ::Logger::Warning(__FILE__, __VA_ARGS__)
#define LOG_ERROR(...) ::Logger::Error  (__FILE__, __VA_ARGS__)
