#pragma once

#include <chrono>
#include <functional>
#include <source_location>
#include <sstream>
#include <string_view>

///
/// \brief Formats and emits timestamped log messages with source-location metadata.
///
/// supports direct function calls (`Log::info("...")`) and stream-style helpers
/// (`Log::Info() << "..."`), where the message is emitted on temporary destruction.
///
namespace Log
{

enum class Level : char
{
   Info = 'i',
   Warning = 'w',
   Error = 'e',
   Fatal = 'f',
};

///
/// \brief Emits an info-level log line.
/// \param message Message text.
/// \param source Callsite source location.
///
void info(const std::string_view& message, const std::source_location& source = std::source_location::current());

///
/// \brief Emits a warning-level log line.
/// \param message Message text.
/// \param source Callsite source location.
///
void warning(const std::string_view& message, const std::source_location& source = std::source_location::current());

///
/// \brief Emits an error-level log line.
/// \param message Message text.
/// \param source Callsite source location.
///
void error(const std::string_view& message, const std::source_location& source = std::source_location::current());

///
/// \brief Emits a fatal-level log line and terminates the process.
/// \param message Message text.
/// \param source Callsite source location.
///
void fatal(const std::string_view& message, const std::source_location& source = std::source_location::current());

using LogFunction = std::function<void(const std::string_view& message, const std::source_location& source)>;

///
/// \brief Stream-style log helper that emits on destruction.
///
struct Message : public std::ostringstream
{
   ///
   /// \brief Creates a message sink bound to a log function and source location.
   /// \param source_location Source location associated with this message.
   /// \param log_function Target log sink function.
   ///
   Message(const std::source_location& source_location, const LogFunction& log_function);

   ///
   /// \brief Flushes buffered stream content through the configured log function.
   ///
   virtual ~Message();

   std::source_location _source_location;
   LogFunction _log_function;
};

///
/// \brief Stream helper for info-level logs.
///
struct Info : public Message
{
   ///
   /// \brief Creates an info-level stream helper for the current callsite.
   /// \param source_location Source location of the log call.
   ///
   Info(const std::source_location& source_location = std::source_location::current());
};

///
/// \brief Stream helper for warning-level logs.
///
struct Warning : public Message
{
   ///
   /// \brief Creates a warning-level stream helper for the current callsite.
   /// \param source_location Source location of the log call.
   ///
   Warning(const std::source_location& source_location = std::source_location::current());
};

///
/// \brief Stream helper for error-level logs.
///
struct Error : public Message
{
   ///
   /// \brief Creates an error-level stream helper for the current callsite.
   /// \param source_location Source location of the log call.
   ///
   Error(const std::source_location& source_location = std::source_location::current());
};

///
/// \brief Stream helper for fatal-level logs.
///
struct Fatal : public Message
{
   ///
   /// \brief Creates a fatal-level stream helper for the current callsite.
   /// \param source_location Source location of the log call.
   ///
   Fatal(const std::source_location& source_location = std::source_location::current());
};

using SysClockTimePoint = std::chrono::time_point<std::chrono::system_clock>;
using ListenerCallback = std::function<void(const SysClockTimePoint&, Level, const std::string&, const std::source_location&)>;

using FlushCallback = std::function<void()>;

///
/// \brief Registers a listener that receives every emitted log message.
/// \param cb Listener callback.
///
void registerListenerCallback(const ListenerCallback& cb);

///
/// \brief Registers a sink flush that fatal() runs before terminating the process.
/// \param cb Flush callback; must complete the write before it returns.
/// \note Without this an asynchronous sink loses the very message that explains the exit,
///       because std::exit tears the runtime down underneath the writing thread.
///
void registerFlushCallback(const FlushCallback& cb);

///
/// \brief Formats a time point as local time in a thread-safe way.
/// \param time_point Time point to format.
/// \param format strftime-style format string.
/// \return Formatted time, or an empty string when the conversion fails.
/// \note std::localtime hands back a pointer to a shared static tm, so two threads formatting a
///       timestamp at once corrupt each other. Log sinks do exactly that: one formats on the
///       thread that emitted the message, another on its own writer thread.
///
std::string formatLocalTime(const SysClockTimePoint& time_point, const std::string& format);

///
/// \brief Builds a compact `file:function:line` source tag.
/// \param source_location Source location to format.
/// \return Formatted source tag.
///
std::string parseSourceTag(const std::source_location& source_location);
}  // namespace Log
