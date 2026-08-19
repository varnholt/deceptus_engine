#include "logthread.h"

// The web build has no filesystem worth logging to, so file logging is compiled out there.
// The Switch does have one: romfs is read-only but libnx mounts the sd card as sdmc:/, and a
// log file there is the only way to see what the game did on real hardware -- stderr reaches
// svcOutputDebugString, which needs a debugger or an emulator attached to read.
#if !defined(DECEPTUS_VRSFML) || defined(__SWITCH__)
#define DECEPTUS_LOG_TO_FILE
#endif

#ifdef DECEPTUS_LOG_TO_FILE
#include "gamepaths.h"
#endif

#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>

#ifdef DECEPTUS_LOG_TO_FILE
namespace
{
int32_t flush_counter = 0;
}
#endif

LogThread::LogThread()
{
#ifdef DECEPTUS_LOG_TO_FILE
   // generate filename with current date
   const auto filename = Log::formatLocalTime(std::chrono::system_clock::now(), "%Y-%m-%d__%H-%M.log");
   const auto log_path = GamePaths::getLogDir() / filename;

   // the stream has to exist before the thread runs, or the first flush dereferences a null _out
   _out = std::make_unique<std::ofstream>(log_path, std::ios::out | std::ios::app);
   if (!_out->is_open())
   {
      std::cerr << "failed to create log file: " << log_path << "\n";
      return;
   }

   _thread = std::make_unique<std::thread>(&LogThread::run, this);
#endif
}

LogThread::~LogThread()
{
#ifdef DECEPTUS_LOG_TO_FILE
   {
      std::lock_guard<std::mutex> guard(_mutex);
      _stopped = true;
   }
   // null when the log file could not be opened, in which case the thread was never started, and
   // already joined when a fatal message came through stop() first
   if (_thread && _thread->joinable())
   {
      _thread->join();
   }

   flush();
#endif
}

void LogThread::log(const SysClockTimePoint& time_point, Log::Level level, const std::string& message, const std::source_location& location)
{
#ifdef DECEPTUS_LOG_TO_FILE
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

#ifdef DECEPTUS_LOG_TO_FILE
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

   std::lock_guard<std::mutex> write_guard(_write_mutex);

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

      const auto now_local = Log::formatLocalTime(timepoint, "%Y-%m-%d %H:%M:%S");

      std::stringstream log_ss;
      log_ss << "[" << static_cast<char>(level) << "] " << now_local << " | " << source_tag << ": " << message;

      *_out << log_ss.str() << std::endl;
   }
}
#endif

void LogThread::flushSynchronously()
{
#ifdef DECEPTUS_LOG_TO_FILE
   // Log::fatal calls this and then std::exit, which unwinds the runtime - including the locale
   // facets basic_filebuf converts through on its way to the file - while this thread is still
   // looping every 100 ms and flushing into it. Flushing alone left that race open: the sink kept
   // writing into a half destroyed runtime and died inside the write, taking the queued messages
   // with it. Stopping and joining the thread first is what closes it.
   stop();
   flush();
#endif
}

void LogThread::stop()
{
#ifdef DECEPTUS_LOG_TO_FILE
   {
      std::lock_guard<std::mutex> guard(_mutex);
      _stopped = true;
   }

   // joining from the logging thread itself would deadlock, and a fatal logged from that thread is
   // exactly the case where that would happen
   if (_thread && _thread->joinable() && _thread->get_id() != std::this_thread::get_id())
   {
      _thread->join();
   }
#endif
}
