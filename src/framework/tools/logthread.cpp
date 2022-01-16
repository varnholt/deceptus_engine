#include "logthread.h"

#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>

#ifdef __GNUC__
#define FMT_HEADER_ONLY
#  include <ctime>
#  include <fmt/core.h>
#else
namespace fmt = std;
#endif


namespace
{
int32_t flush_counter = 0;
}


LogThread::LogThread()
{
   // generate filename with current date
   const auto now = std::chrono::system_clock::now();
   const auto now_time = std::chrono::system_clock::to_time_t(now);
   std::stringstream ss;
   ss << std::put_time(std::localtime(&now_time), "%Y-%m-%d__%H-%M.log");
   const auto filename = ss.str();

   _thread = std::make_unique<std::thread>(&LogThread::run, this);
   _out = std::make_unique<std::ofstream>(filename);
}


LogThread::~LogThread()
{
   _stopped = true;
   _thread->join();
   flush();
}


void LogThread::log(
   const SysClockTimePoint& time_point,
   Log::Level level,
   const std::string& message,
   const std::source_location& location
)
{
   std::lock_guard<std::mutex> guard(_mutex);
   _log_items.push_back(LogItem{
         time_point,
         level,
         message,
         location
      }
   );
}


void LogThread::run()
{
   while (!_stopped)
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      flush_counter++;

      if (flush_counter == 100 || _log_items.size() == 10)
      {
         flush();
         flush_counter = 0;
      }
   }
}


void LogThread::flush()
{
   std::vector<LogItem> copy;
   {
      std::lock_guard<std::mutex> guard(_mutex);
      std::copy(_log_items.begin(), _log_items.end(), std::back_inserter(copy));
      _log_items.clear();
   }

   for (auto& item : copy)
   {
      const auto& timepoint = item._timepoint;
      const auto level = item._level;
      const auto& message = item._message;
      const auto& source_location = item._source_location;

      const auto source_tag = fmt::format(
         "{0}:{1}:{2}",
         std::filesystem::path{source_location.file_name()}.filename().string(),
         source_location.function_name(),
         source_location.line()
      );

#ifdef __GNUC__
      const auto now_time = std::chrono::system_clock::to_time_t(now);

      std::stringstream ss;
      ss << std::put_time(std::localtime(&now_time), "%Y-%m-%d %X");
      const auto now_local = ss.str();
#else
      const auto now_local = std::chrono::zoned_time{std::chrono::current_zone(), timepoint};
#endif

      *_out
         << fmt::format(
#ifdef __GNUC__
            "[{0}] {1} | {2}: {3}",
#else
            "[{0}] {1:%T} | {2}: {3}",
#endif
            static_cast<char>(level),
            now_local,
            source_tag,
            message
         )
         << std::endl;
   }
}


