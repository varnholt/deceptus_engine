#pragma once

#include <chrono>
#include <fstream>
#include <memory>
#include <mutex>
#include <source_location>
#include <string>
#include <thread>

#include "log.h"

class LogThread
{
public:
   LogThread();
   virtual ~LogThread();

   using SysClockTimePoint = std::chrono::time_point<std::chrono::system_clock>;
   void log(const SysClockTimePoint&, Log::Level, const std::string&, const std::source_location&);

private:
   struct LogItem
   {
      SysClockTimePoint _timepoint;
      Log::Level _level;
      std::string _message;
      std::source_location _source_location;
   };

   void run();
   void flush();

   std::mutex _mutex;
   std::vector<LogItem> _log_items;

   std::unique_ptr<std::thread> _thread;
   std::unique_ptr<std::ofstream> _out;
   bool _stopped = false;
};
