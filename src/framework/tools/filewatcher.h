#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>

//! \brief watches a single file and reports whether it has been modified since watching started.
//!
//! Polls the file's last write time on a background thread. The poll interval is waited out on a
//! condition variable rather than slept through, so stop() - and therefore the destructor - returns
//! immediately instead of blocking for up to a full interval. That matters wherever the owning
//! object is destroyed on a thread that must not stall, such as a render loop.
class FileWatcher
{
public:
   FileWatcher() = default;
   ~FileWatcher();

   FileWatcher(const FileWatcher&) = delete;
   FileWatcher& operator=(const FileWatcher&) = delete;

   /// \brief starts watching a file; does nothing if already watching.
   /// \param path file to watch.
   /// \param description name used when logging a detected change.
   /// \param poll_interval how often the write time is checked.
   void start(
      const std::filesystem::path& path,
      const std::string& description,
      std::chrono::milliseconds poll_interval = std::chrono::seconds{1}
   );

   /// \brief stops watching and joins the thread; safe to call more than once.
   void stop();

   /// \brief returns whether the file changed since start() was called.
   bool modified() const;

private:
   void poll();

   std::filesystem::path _path;
   std::string _description;
   std::chrono::milliseconds _poll_interval{std::chrono::seconds{1}};

   std::thread _thread;
   std::atomic<bool> _active{false};
   std::atomic<bool> _modified{false};
   std::mutex _mutex;
   std::condition_variable _stop_condition;
};
