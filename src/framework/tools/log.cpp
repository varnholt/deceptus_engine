#include "log.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <source_location>

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
constexpr bool colored_output = true;

// ansi color codes
constexpr const char* color_reset = "\033[0m";
constexpr const char* color_yellow = "\033[33m";
constexpr const char* color_red = "\033[31m";
constexpr const char* color_bright_red = "\033[91m";

std::vector<Log::ListenerCallback> _log_callbacks;

// enable ansi colors on windows console
void enableWindowsAnsiColors()
{
#ifdef _WIN32
   static bool initialized = false;
   if (!initialized)
   {
      HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
      if (h_out != INVALID_HANDLE_VALUE)
      {
         DWORD dw_mode = 0;
         if (GetConsoleMode(h_out, &dw_mode))
         {
            dw_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(h_out, dw_mode);
         }
      }
      initialized = true;
   }
#endif
}

constexpr const char* getColorForLevel(Log::Level level)
{
   switch (level)
   {
      case Log::Level::Warning:
         return color_yellow;
      case Log::Level::Error:
         return color_red;
      case Log::Level::Fatal:
         return color_bright_red;
      default:
         return "";
   }
}

std::string formatTime(const std::chrono::system_clock::time_point& now)
{
   return std::format("{:%Y-%m-%d %H:%M:%S}", std::chrono::system_clock::now());
}

void log(Log::Level level, const std::string_view& message, const std::source_location& source_location)
{
   const auto now = std::chrono::system_clock::now();
   const auto source_tag = Log::parseSourceTag(source_location);
   const auto now_local = formatTime(now);

   if constexpr (colored_output)
   {
      enableWindowsAnsiColors();
      const char* color = getColorForLevel(level);
      std::cout << color << "[" << static_cast<char>(level) << "] " << now_local << " | " << source_tag << ": " << message << color_reset
                << std::endl;
   }
   else
   {
      std::cout << "[" << static_cast<char>(level) << "] " << now_local << " | " << source_tag << ": " << message << std::endl;
   }

   for (const auto& callback : _log_callbacks)
   {
      callback(now, level, std::string{message}, source_location);
   }
}

}  // namespace

void Log::registerListenerCallback(const ListenerCallback& cb)
{
   _log_callbacks.push_back(cb);
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

std::string Log::parseSourceTag(const std::source_location& source_location)
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

   const auto source_tag = std::filesystem::path{source_location.file_name()}.filename().string() + ":" + function_name + ":" +
                           std::to_string(source_location.line());

   return source_tag;
}
