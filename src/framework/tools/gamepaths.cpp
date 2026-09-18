#include "gamepaths.h"

#include "framework/tools/assetsource.h"

#include <cstdlib>
#include <fstream>
#include <stdexcept>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace
{

// DEVELOPMENT_MODE builds write to a separate "deceptus-dev" tree rather than sharing "deceptus"
// with release builds. Settings, save state, logs, recordings, and crash dumps all hang off
// getGameDataDir(), so without this a settings tweak made while testing a dev build (e.g. forcing
// a small windowed size to script screenshots) silently becomes what a release build starts up
// with next, and vice versa - the two builds have no business sharing mutable state.
#ifdef DEVELOPMENT_MODE
constexpr auto* deceptus_folder_name = "deceptus-dev";
#else
constexpr auto* deceptus_folder_name = "deceptus";
#endif

}  // namespace

namespace GamePaths
{

std::filesystem::path getGameDataDir()
{
#ifdef __EMSCRIPTEN__
   // on the web the writable tree lives inside the IDBFS mount created in main(), which is
   // synchronized to IndexedDB so it survives page reloads
   return std::filesystem::path("/") / deceptus_folder_name;
#elif defined(__SWITCH__)
   // romfs is read-only, so saves go to the sd card; libnx mounts it as sdmc:/
   return std::filesystem::path("sdmc:/switch") / deceptus_folder_name;
#elif defined(_WIN32)
   // on windows, use %APPDATA%\deceptus
   const char* appdata_folder = std::getenv("APPDATA");
   if (appdata_folder)
   {
      return std::filesystem::path(appdata_folder) / deceptus_folder_name;
   }
   else
   {
      // fall back to user home directory
      const char* home_folder = std::getenv("USERPROFILE");
      if (home_folder)
      {
         return std::filesystem::path(home_folder) / (std::string(".") + deceptus_folder_name);
      }
   }
#else
   // on Linux/macOS, use ~/.local/share/deceptus
   const char* home = std::getenv("HOME");
   if (home)
   {
      return std::filesystem::path(home) / ".local" / "share" / deceptus_folder_name;
   }
#endif

   // if environment variables are not available, return a default path in current directory
   return std::filesystem::path(".") / deceptus_folder_name;
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
#ifdef __SWITCH__
      if (std::filesystem::exists(bundled_default))
      {
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
      }
#else
      // bundled_default may live inside the shipping-mode packed archive rather than on disk, so
      // it has to be read through AssetSource rather than std::filesystem::copy_file.
      const auto bundled_default_contents = AssetSource::readFile(bundled_default);
      if (bundled_default_contents.has_value())
      {
         std::ofstream destination(target, std::ios::binary | std::ios::trunc);
         destination.write(bundled_default_contents->data(), static_cast<std::streamsize>(bundled_default_contents->size()));
      }
#endif
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
