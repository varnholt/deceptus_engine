#pragma once

#include <atomic>
#include <chrono>
#include <fstream>
#include <memory>
#include <mutex>
#include <source_location>
#include <string>
#include <thread>

#include "log.h"

///
/// \brief writes log messages to a rotating file from a background thread.
///
class LogThread
{
public:
   ///
   /// \brief starts the logging thread and opens a timestamped log file.
   ///
   LogThread();

   ///
   /// \brief stops the logging thread and flushes pending log items.
   ///
   virtual ~LogThread();

   using SysClockTimePoint = std::chrono::time_point<std::chrono::system_clock>;

   ///
   /// \brief queues one log entry for asynchronous file output.
   /// \param time_point message timestamp.
   /// \param level log severity level.
   /// \param message formatted message text.
   /// \param location source location for the message.
   ///
   void log(const SysClockTimePoint& time_point, Log::Level level, const std::string& message, const std::source_location& location);

   ///
   /// \brief writes every queued record to disk before returning.
   /// \note registered as Log's flush callback so a fatal message reaches the file before
   ///       std::exit tears the runtime down under the background thread.
   ///
   void flushSynchronously();

private:
   ///
   /// \brief one queued log record.
   ///
   struct LogItem
   {
      SysClockTimePoint _timepoint;
      Log::Level _level;
      std::string _message;
      std::source_location _source_location;
   };

   ///
   /// \brief runs the background loop and flushes periodically.
   ///
   void run();

   ///
   /// \brief writes queued log records to disk.
   ///
   void flush();

   std::mutex _mutex;
   std::mutex _write_mutex;  //!< serializes the file write, which flushSynchronously does from another thread
   std::vector<LogItem> _log_items;

   std::unique_ptr<std::thread> _thread;
   std::unique_ptr<std::ofstream> _out;
   std::atomic<bool> _stopped = false;
};
