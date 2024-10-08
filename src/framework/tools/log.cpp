#include "log.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <source_location>

namespace
{
Log::ListenerCallback _log_callback;

std::string formatTime(const std::chrono::system_clock::time_point& now)
{
   const auto zoned_time = std::chrono::zoned_time{std::chrono::current_zone(), now};
   return std::format("{:%Y-%m-%d %H:%M:%S}", zoned_time);
}

void log(Log::Level level, const std::string_view& message, const std::source_location& source_location)
{
   std::string function_name = source_location.function_name();
   function_name = function_name.substr(0, function_name.find('('));

   // remove '__cdecl' if it exists
   const std::string cdecl_str = "__cdecl ";
   size_t cdecl_pos = function_name.find(cdecl_str);
   if (cdecl_pos != std::string::npos)
   {
      function_name.erase(cdecl_pos, cdecl_str.length());
   }

   const auto now = std::chrono::system_clock::now();
   const auto source_tag = std::filesystem::path{source_location.file_name()}.filename().string() + ":" + function_name + ":" +
                           std::to_string(source_location.line());

   const auto now_local = formatTime(now);

   std::cout << "[" << static_cast<char>(level) << "] " << now_local << " | " << source_tag << ": " << message << std::endl;

   if (_log_callback)
   {
      _log_callback(now, level, std::string{message}, source_location);
   }
}

}  // namespace

void Log::registerListenerCallback(const ListenerCallback& cb)
{
   _log_callback = cb;
}

void Log::info(const std::string_view& message, const std::source_location& source_location)
{
   log(Level::Info, message, source_location);
}

void Log::warning(const std::string_view& message, const std::source_location& source_location)
{
   log(Level::Warning, message, source_location);
}

void Log::error(const std::string_view& message, const std::source_location& source_location)
{
   log(Level::Error, message, source_location);
}

void Log::fatal(const std::string_view& message, const std::source_location& source_location)
{
   log(Level::Fatal, message, source_location);
   std::exit(-1);
}

Log::Message::Message(const std::source_location& source_location, const LogFunction& log_function)
    : _source_location(source_location), _log_function(log_function)
{
}

Log::Message::~Message()
{
   _log_function(str(), _source_location);
}

Log::Info::Info(const std::source_location& source_location) : Message(source_location, info)
{
}

Log::Warning::Warning(const std::source_location& source_location) : Message(source_location, warning)
{
}

Log::Error::Error(const std::source_location& source_location) : Message(source_location, error)
{
}

Log::Fatal::Fatal(const std::source_location& source_location) : Message(source_location, fatal)
{
}
