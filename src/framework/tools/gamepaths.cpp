#include "gamepaths.h"
#include <cstdlib>
#include <stdexcept>

#ifdef __SWITCH__
#include <fstream>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace GamePaths
{

std::filesystem::path getGameDataDir()
{
#ifdef __EMSCRIPTEN__
   // on the web the writable tree lives inside the IDBFS mount created in main(), which is
   // synchronized to IndexedDB so it survives page reloads
   return std::filesystem::path("/deceptus");
#elif defined(__SWITCH__)
   // romfs is read-only, so saves go to the sd card; libnx mounts it as sdmc:/
   return std::filesystem::path("sdmc:/switch/deceptus");
#elif defined(_WIN32)
   // on windows, use %APPDATA%\deceptus
   const char* appdata_folder = std::getenv("APPDATA");
   if (appdata_folder)
   {
      return std::filesystem::path(appdata_folder) / "deceptus";
   }
   else
   {
      // fall back to user home directory
      const char* home_folder = std::getenv("USERPROFILE");
      if (home_folder)
      {
         return std::filesystem::path(home_folder) / ".deceptus";
      }
   }
#else
   // on Linux/macOS, use ~/.local/share/deceptus
   const char* home = std::getenv("HOME");
   if (home)
   {
      return std::filesystem::path(home) / ".local" / "share" / "deceptus";
   }
#endif

   // if environment variables are not available, return a default path in current directory
   return std::filesystem::path(".") / "deceptus";
}

std::filesystem::path getSettingsDir()
{
   auto settings_dir = getGameDataDir() / "settings";
   std::filesystem::create_directories(settings_dir);
   return settings_dir;
}

std::filesystem::path getPreferencesFile(const std::string& filename)
{
   const auto target = getSettingsDir() / filename;

   // seed the writable copy from the bundled default on first access; on desktop this also migrates
   // progress written by earlier versions that still lived in the read-only data/config tree
   if (!std::filesystem::exists(target))
   {
      const auto bundled_default = std::filesystem::path("data/config") / filename;
      if (std::filesystem::exists(bundled_default))
      {
#ifdef __SWITCH__
         // the source lives in romfs and the target on the sd card, which are two separate
         // devoptab devices. newlib's copy_file cannot move data between them: it creates
         // the target, copies nothing, and reports failure through the error_code that the
         // desktop path discards. an empty config then takes the game down at startup,
         // because GameConfiguration::deserialize calls json::parse outside its try block.
         // copying through streams sidesteps the device boundary entirely.
         {
            std::ifstream source(bundled_default, std::ios::binary);
            std::ofstream destination(target, std::ios::binary | std::ios::trunc);
            destination << source.rdbuf();
         }

         // never leave a truncated config behind; the next launch would inherit it and the
         // failure would look like a corrupt install rather than a failed copy
         std::error_code size_error;
         if (std::filesystem::file_size(target, size_error) == 0 || size_error)
         {
            std::error_code remove_error;
            std::filesystem::remove(target, remove_error);
         }
#else
         std::error_code error_code;
         std::filesystem::copy_file(bundled_default, target, error_code);
#endif
      }
   }

   return target;
}

void flushToPersistentStorage()
{
#ifdef __EMSCRIPTEN__
   // persist the IDBFS mount back to IndexedDB so the write survives a page reload
   EM_ASM(FS.syncfs(
      false,
      function(error) {
         if (error)
         {
            console.error("FS.syncfs failed:", error);
         }
      }
   ););
#endif
}

std::filesystem::path getLogDir()
{
   auto log_dir = getGameDataDir() / "logs";
   std::filesystem::create_directories(log_dir);
   return log_dir;
}

std::filesystem::path getRecordingDir()
{
   auto recording_dir = getGameDataDir() / "recordings";
   std::filesystem::create_directories(recording_dir);
   return recording_dir;
}

void createGameDirectories()
{
   getSettingsDir();
   getLogDir();
   getRecordingDir();
}

}  // namespace GamePaths
