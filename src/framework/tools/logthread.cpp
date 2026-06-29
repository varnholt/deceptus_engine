#include "logthread.h"

#ifndef __EMSCRIPTEN__
#include "gamepaths.h"
#endif

#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>

#ifndef __EMSCRIPTEN__
namespace
{
int32_t flush_counter = 0;
}
#endif

LogThread::LogThread()
{
#ifndef __EMSCRIPTEN__
   // generate filename with current date
   const auto now = std::chrono::system_clock::now();
   const auto now_time = std::chrono::system_clock::to_time_t(now);
   std::stringstream output_stream;
   output_stream << std::put_time(std::localtime(&now_time), "%Y-%m-%d__%H-%M.log");
   const auto filename = output_stream.str();
   const auto log_path = GamePaths::getLogDir() / filename;

   _thread = std::make_unique<std::thread>(&LogThread::run, this);
   _out = std::make_unique<std::ofstream>(log_path, std::ios::out | std::ios::app);
   if (!_out->is_open())
   {
      std::cerr << "failed to create log file: " << log_path << "\n";
      return;
   }
#endif
}

LogThread::~LogThread()
{
#ifndef __EMSCRIPTEN__
   {
      std::lock_guard<std::mutex> guard(_mutex);
      _stopped = true;
   }
   _thread->join();
   flush();
#endif
}

void LogThread::log(const SysClockTimePoint& time_point, Log::Level level, const std::string& message, const std::source_location& location)
{
#ifndef __EMSCRIPTEN__
   std::lock_guard<std::mutex> guard(_mutex);
   if (_stopped)
   {
      return;
   }
   _log_items.push_back(LogItem{time_point, level, message, location});
#else
   (void)time_point;
   (void)level;
   (void)message;
   (void)location;
#endif
}

#ifndef __EMSCRIPTEN__
void LogThread::run()
{
   while (!_stopped)
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      flush_counter++;

      bool should_flush = false;
      {
         std::lock_guard<std::mutex> guard(_mutex);
         should_flush = (flush_counter == 100) || (_log_items.size() >= 10);
      }
      if (should_flush)
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

   for (const auto& item : copy)
   {
      const auto& timepoint = item._timepoint;
      const auto level = item._level;
      const auto& message = item._message;
      const auto& source_location = item._source_location;

      std::stringstream source_tag_ss;
      source_tag_ss << std::filesystem::path{source_location.file_name()}.filename().string() << ":" << source_location.function_name()
                    << ":" << source_location.line();
      const auto source_tag = source_tag_ss.str();

      const auto now_time = std::chrono::system_clock::to_time_t(timepoint);
      std::stringstream time_ss;
      time_ss << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S");
      const auto now_local = time_ss.str();

      std::stringstream log_ss;
      log_ss << "[" << static_cast<char>(level) << "] " << now_local << " | " << source_tag << ": " << message;

      *_out << log_ss.str() << std::endl;
   }
}
#endif
