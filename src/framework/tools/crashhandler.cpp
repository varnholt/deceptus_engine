#include "crashhandler.h"

#ifdef _WIN32

#include <windows.h>
// dbghelp.h has to come after windows.h
#include <dbghelp.h>

#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>

#include "framework/tools/gamepaths.h"

namespace
{

std::filesystem::path dumpPath()
{
   auto directory = GamePaths::getGameDataDir() / "crashdumps";
   std::error_code error;
   std::filesystem::create_directories(directory, error);

   const auto now = std::time(nullptr);
   std::tm local{};
   localtime_s(&local, &now);

   std::ostringstream filename;
   filename << std::put_time(&local, "%Y-%m-%d__%H-%M-%S") << ".dmp";
   return directory / filename.str();
}

LONG WINAPI writeMiniDump(EXCEPTION_POINTERS* exception_pointers)
{
   const auto path = dumpPath();

   auto* file = CreateFileW(path.wstring().c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
   if (file != INVALID_HANDLE_VALUE)
   {
      MINIDUMP_EXCEPTION_INFORMATION exception_info{};
      exception_info.ThreadId = GetCurrentThreadId();
      exception_info.ExceptionPointers = exception_pointers;
      exception_info.ClientPointers = FALSE;

      // MiniDumpWithThreadInfo and the memory ranges keep enough context to walk every thread's
      // stack afterwards, which is the point: these crashes only happen with no debugger attached.
      const auto type = static_cast<MINIDUMP_TYPE>(
         MiniDumpWithThreadInfo | MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithDataSegs | MiniDumpWithHandleData
      );

      MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, type, &exception_info, nullptr, nullptr);
      CloseHandle(file);

      // the log thread is very likely gone by now, so report on stderr where it is still visible
      fwprintf(stderr, L"\ncrash dump written to %s\n", path.wstring().c_str());
      fflush(stderr);
   }

   return EXCEPTION_EXECUTE_HANDLER;
}

}  // namespace

namespace CrashHandler
{

void install()
{
   SetUnhandledExceptionFilter(writeMiniDump);
}

}  // namespace CrashHandler

#else

namespace CrashHandler
{
void install()
{
}
}  // namespace CrashHandler

#endif
