#include "game/game.h"

#ifdef _WIN32 && !defined(DEBUG)
#include <windows.h>
#endif

#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#ifdef __SWITCH__
#include <switch.h>
#include <unistd.h>
#endif

#include "framework/tools/crashhandler.h"
#include "framework/tools/gamepaths.h"
#include "framework/tools/localization.h"
#include "framework/tools/logthread.h"
#include "game/config/localizationloader.h"
#include "game/constants.h"
#include "game/debug/logui.h"
#include "game/io/preloader.h"
#include "game/tests/test.h"

#ifdef __linux__
extern "C" int XInitThreads();
#endif

void debugAuthors()
{
   const std::chrono::time_point<std::chrono::system_clock> now{std::chrono::system_clock::now()};
   const std::chrono::year_month_day ymd{std::chrono::floor<std::chrono::days>(now)};

   std::stringstream text;
   text << GAME_NAME;
   text << " (c) ";
   text << static_cast<int32_t>(ymd.year());
   text << " dstar/mueslee";

   std::cout << text.str() << std::endl;
   for (auto i = 0u; i < text.str().length(); i++)
   {
      std::cout << "-";
   }
   std::cout << std::endl;
}

#if defined(_WIN32) && !defined(DEBUG)
int WINAPI WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/)
#else
int main(int /*argc*/, char** /*argv*/)
#endif
{
#ifdef __SWITCH__
   // the assets are baked into the .nro as romfs. mount it and make it the working
   // directory so the engine's relative "data/..." paths resolve, before anything tries
   // to read a config or a texture. saves go elsewhere: romfs is read-only, see
   // GamePaths::getGameDataDir().
   if (R_FAILED(romfsInit()))
   {
      return 1;
   }

   if (chdir("romfs:/") != 0)
   {
      romfsExit();
      return 1;
   }

   // route stdout through svcOutputDebugString so the engine's log is visible to a
   // debugger or emulator. libnx points stderr at the debug device, and stdout is
   // aliased onto it so std::cout goes the same way. harmless on real hardware with
   // nothing attached, and the only way to see why startup fails without nxlink.
   consoleDebugInit(debugDevice_SVC);
   stdout = stderr;
#endif

#ifdef __EMSCRIPTEN__
   // mount a persistent IDBFS-backed filesystem and load its contents from IndexedDB before any
   // settings or save files are resolved. ASYNCIFY lets us block until the initial load completes,
   // so GamePaths sees the persisted data on startup.
   EM_ASM(FS.mkdir('/deceptus'); FS.mount(IDBFS, {}, '/deceptus'); Module.__persistent_fs_synced = 0; FS.syncfs(
      true,
      function(error) {
         if (error)
         {
            console.error("initial FS.syncfs failed:", error);
         }
         Module.__persistent_fs_synced = 1;
      }
   ););
   while (emscripten_run_script_int("Module.__persistent_fs_synced") == 0)
   {
      emscripten_sleep(50);
   }
#endif

#ifdef __SWITCH__
// temporary startup tracing; stderr reaches svcOutputDebugString via consoleDebugInit,
// which std::cout does not, because its filebuf caches the original FILE*
#define SWITCH_TRACE(step) \
   do                      \
   {                       \
      fprintf(stderr, "[switch-trace] %s\n", step); \
   } while (0)
#else
#define SWITCH_TRACE(step) \
   do                      \
   {                       \
   } while (0)
#endif

   SWITCH_TRACE("entered main");

   CrashHandler::install();

   GamePaths::createGameDirectories();

   SWITCH_TRACE("game directories created");

   // setup logging to file
   // weak_ptr: game threads may still fire log callbacks after main() unwinds and log_thread is destroyed;
   // lock() returns null once the shared_ptr drops, making the callback a safe no-op instead of a use-after-free.
   auto log_thread = std::make_shared<LogThread>();
   std::weak_ptr<LogThread> log_thread_weak(log_thread);
   Log::registerListenerCallback(
      [log_thread_weak](const auto& time_point, auto level, const auto& message, const auto& location)
      {
         if (auto log_thread = log_thread_weak.lock())
         {
            log_thread->log(time_point, level, message, location);
         }
      }
   );

#ifdef DEVELOPMENT_MODE
   Log::registerListenerCallback([](const auto& time_point, auto level, const auto& message, const auto& location)
                                 { LogUiBuffer::log(time_point, level, message, location); });
#endif

#ifdef __linux__
   XInitThreads();
#endif

   SWITCH_TRACE("logging installed");

   LocalizationLoader::loadFromConfig();
   debugAuthors();

   SWITCH_TRACE("localization loaded");

#ifdef DECEPTUS_VRSFML
   auto graphics_context = sf::GraphicsContext::create();
   SWITCH_TRACE("graphics context created");
   auto audio_context = sf::AudioContext::create();
   SWITCH_TRACE("audio context created");
#endif

   Test test;
   Game game;
   SWITCH_TRACE("game constructed");
   game.initialize();
   SWITCH_TRACE("game initialized");
   Preloader::preload();
   SWITCH_TRACE("preload done, entering loop");
   const auto result = game.loop();
   SWITCH_TRACE("loop returned");

#ifdef DEVELOPMENT_MODE
   Localization::getInstance().flushMissingKeys();
#endif

   return result;
}
