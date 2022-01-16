#pragma once

#include <chrono>
#include <functional>
#include <source_location>
#include <sstream>
#include <string_view>


/*!
 * Log functions to output messages in a consistent manner including a timestamp and source location.
 * In contrast to std::cout << "bla", a newline is always appended to each message.
 *
 * usages
 *    Log::info("Hello World");
 *    Log::Info() << "Hello " << "World";
 */
namespace Log
{

enum class Level : char
{
   Info    = 'i',
   Warning = 'w',
   Error   = 'e'
};

void info(const std::string_view& message, const std::source_location& source = std::source_location::current());
void warning(const std::string_view& message, const std::source_location& source = std::source_location::current());
void error(const std::string_view& message, const std::source_location& source = std::source_location::current());

using LogFunction = std::function<void(const std::string_view& message, const std::source_location& source)>;

struct Message : public std::ostringstream
{
   Message(const std::source_location& source_location, const LogFunction& log_function);
   virtual ~Message();
   std::source_location _source_location;
   LogFunction _log_function;
};

struct Info : public Message{Info(const std::source_location& source_location = std::source_location::current());};
struct Warning : public Message{Warning(const std::source_location& source_location = std::source_location::current());};
struct Error : public Message{Error(const std::source_location& source_location = std::source_location::current());};

using SysClockTimePoint = std::chrono::time_point<std::chrono::system_clock>;
using ListenerCallback = std::function<void(const SysClockTimePoint&, Level, const std::string&, const std::source_location&)>;
void registerListenerCallback(const ListenerCallback& cb);
}

