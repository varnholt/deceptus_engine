#include "filewatcher.h"

#include "framework/tools/log.h"
#if defined(DEVELOPMENT_MODE) && !defined(DECEPTUS_VRSFML)
#include <fstream>
#include <iterator>

namespace
{
std::optional<std::string> readContents(const std::filesystem::path& path)
{
   std::ifstream file(path, std::ios::binary);
   if (!file) return std::nullopt;
   return std::string{std::istreambuf_iterator<char>{file}, {}};
}
}

void FileWatcher::acceptInspectorContents(const std::string& contents)
{
   std::lock_guard lock(_mutex);
   _inspector_contents = contents;
   if (readContents(_path) == _inspector_contents)
   {
      _modified = false;
   }
}
#endif

FileWatcher::~FileWatcher()
{
   stop();
}

void FileWatcher::start(const std::filesystem::path& path, const std::string& description, std::chrono::milliseconds poll_interval)
{
   if (_active)
   {
      return;
   }

   _path = path;
   _description = description;
   _poll_interval = poll_interval;
   _modified = false;
#if defined(DEVELOPMENT_MODE) && !defined(DECEPTUS_VRSFML)
   _inspector_contents.reset();
#endif
   _active = true;

   _thread = std::thread([this]() { poll(); });
}

void FileWatcher::stop()
{
   _active = false;
   _stop_condition.notify_all();

   if (_thread.joinable())
   {
      _thread.join();
   }
}

bool FileWatcher::modified() const
{
   return _modified;
}

void FileWatcher::poll()
{
   // the error_code overload throughout: a file being rewritten right now can fail to stat, and an
   // exception escaping this thread would take the process down
   std::error_code error;
   auto reference_time = std::filesystem::last_write_time(_path, error);

   while (_active)
   {
      const auto current_time = std::filesystem::last_write_time(_path, error);
      if (!error && current_time != reference_time)
      {
#if defined(DEVELOPMENT_MODE) && !defined(DECEPTUS_VRSFML)
         std::lock_guard lock(_mutex);
         const auto inspector_write = _inspector_contents && readContents(_path) == _inspector_contents;
         _inspector_contents.reset();
         if (!inspector_write)
         {
            Log::Info() << _description << " was modified, marking as dirty";
            _modified = true;
         }
#else
         Log::Info() << _description << " was modified, marking as dirty";
         _modified = true;
#endif
         reference_time = current_time;
      }

      std::unique_lock<std::mutex> lock(_mutex);
      _stop_condition.wait_for(lock, _poll_interval, [this]() { return !_active.load(); });
   }
}
