#pragma once

/// \brief writes a minidump when the process dies from an unhandled exception.
namespace CrashHandler
{

/// \brief installs the unhandled exception filter.
/// \details on windows an access violation that reaches the top of the stack normally kills the
///          process silently, leaving nothing to inspect unless a debugger happened to be attached.
///          this writes a minidump to <appdata>/deceptus/crashdumps so a crash that only reproduces
///          without a debugger can still be analysed afterwards. a no-op on other platforms.
void install();

}  // namespace CrashHandler
