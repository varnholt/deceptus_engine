#include "gamepaths.h"
#include <cstdlib>
#include <stdexcept>

namespace GamePaths
{

std::filesystem::path getGameDataDir()
{
#ifdef _WIN32
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
