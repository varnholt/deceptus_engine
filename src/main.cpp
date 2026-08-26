#include "game/game.h"

#ifdef _WIN32 && !defined(DEBUG)
#include <windows.h>
#endif

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
#include <string_view>

#ifdef DECEPTUS_VRSFML
#include <SFML/System/Err.hpp>
#endif

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

#ifdef _WIN32
// On a laptop with both an integrated and a discrete GPU, Windows hands an OpenGL process the
// integrated one unless it is told otherwise. Both vendors look for these exported symbols while the
// process is loaded, before any context exists, and give it the discrete GPU when they are set.
// A per application preference in the windows graphics settings still overrides this, which is what
// the GPU row in the video options writes.
extern "C"
{
   __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
   __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

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

   CrashHandler::install();

   GamePaths::createGameDirectories();

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

   Log::registerFlushCallback(
      [log_thread_weak]
      {
         if (auto log_thread = log_thread_weak.lock())
         {
            log_thread->flushSynchronously();
         }
      }
   );

   // main() is not the only way out: Game::shutdown, the lua error handlers and a handful of
   // unrecoverable asset failures all call exit() directly. That skips log_thread's destructor,
   // so the writer thread is still flushing every 100 ms while the runtime tears itself down
   // around it - and on the switch it dies inside that write, taking every queued message with
   // it. The log then ends mid-startup and the reason for the exit is never written, which is
   // exactly the situation the log exists for. Registering here rather than at static init time
   // is deliberate: exit handlers run in reverse order of registration, so this one runs before
   // the statics that logging itself depends on are gone.
   std::atexit([] { Log::flush(); });

#ifdef DEVELOPMENT_MODE
   Log::registerListenerCallback([](const auto& time_point, auto level, const auto& message, const auto& location)
                                 { LogUiBuffer::log(time_point, level, message, location); });
#endif

#ifdef __linux__
   XInitThreads();
#endif

#ifdef DECEPTUS_VRSFML
   // VRSFML writes its own errors straight to stderr, which never reaches the game log. On a
   // console that means they are invisible: there is no terminal, and stderr only goes to
   // svcOutputDebugString, which needs a debugger or an emulator attached to read. Routing
   // them into Log puts them in the same file as everything else -- on the Switch, on the sd
   // card, which is the only artefact a hardware run leaves behind.
   sf::priv::setErrSink(
      [](void*, const char* data, std::size_t size)
      {
         // the sink is also called for the lone trailing newline after a message
         std::string_view text{data, size};
         while (!text.empty() && (text.back() == '\n' || text.back() == '\r'))
         {
            text.remove_suffix(1);
         }

         if (!text.empty())
         {
            Log::error(text);
         }
      },
      nullptr
   );
#endif

#ifdef __SWITCH__
   // How much memory homebrew gets depends entirely on how it was launched. Started from the
   // album, it runs as a library applet inside that applet's small memory pool; started in
   // title takeover mode -- hold R while launching a game from the home menu -- it gets the
   // whole application pool instead, which is an order of magnitude larger. This game loads
   // over a hundred megabytes of assets, so the difference decides whether it runs at all,
   // and the failure mode is unhelpful: an image fails to decode with "outofmem" somewhere
   // deep in start-up. Logging the numbers up front turns that into an obvious diagnosis.
   {
      u64 total_memory_size = 0;
      u64 used_memory_size = 0;
      svcGetInfo(&total_memory_size, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0);
      svcGetInfo(&used_memory_size, InfoType_UsedMemorySize, CUR_PROCESS_HANDLE, 0);

      const auto applet_type = appletGetAppletType();
      const auto title_takeover = (applet_type == AppletType_Application || applet_type == AppletType_SystemApplication);

      Log::Info() << "switch: applet type " << static_cast<int32_t>(applet_type) << ", memory " << (used_memory_size / (1024 * 1024))
                  << " MB used of " << (total_memory_size / (1024 * 1024)) << " MB";

      if (!title_takeover)
      {
         Log::Warning() << "switch: running in applet mode, which reserves only a fraction of the console's memory. "
                           "If asset loading fails, relaunch in title takeover mode: hold R while starting a game "
                           "from the home menu, then run this from the homebrew menu that opens.";
      }
   }
#endif

   LocalizationLoader::loadFromConfig();
   debugAuthors();

#ifdef DECEPTUS_VRSFML
   auto graphics_context = sf::GraphicsContext::create();
   auto audio_context = sf::AudioContext::create();
#endif

   Test test;
   Game game;
   game.initialize();
   Preloader::preload();
   const auto result = game.loop();

#ifdef DEVELOPMENT_MODE
   Localization::getInstance().flushMissingKeys();
#endif

   return result;
}
