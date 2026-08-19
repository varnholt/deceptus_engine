@echo off
rem Runs the Nintendo Switch build in the Ryujinx emulator.
rem
rem   run_switch.bat                     -> build_switch_engine\deceptus.nro
rem   run_switch.bat path\to\other.nro   -> that one instead
rem
rem Adjust these two if the emulator or the build lives somewhere else.
set RYUJINX=D:\games\ryujinx-1.3.2-win_x64\publish\Ryujinx.exe
set NRO=%~dp0build_switch_engine\deceptus.nro

rem One-time emulator setup, in case this is a fresh Ryujinx install:
rem   - copy prod.keys into %APPDATA%\Ryujinx\system\ (without them nothing boots)
rem   - set "update_checker_type": "Off" in %APPDATA%\Ryujinx\Config.json, otherwise a
rem     GitHub 404 dialog blocks startup and the window never appears
rem
rem Mind that Ryujinx is no performance proxy: it JITs the guest onto a desktop cpu and
rem re-issues the Tegra command buffers on a desktop gpu. Use it to see whether something
rem works, not how fast it is.

if not "%~1" == "" set NRO=%~1

if not exist "%RYUJINX%" (
    echo emulator not found at %RYUJINX%
    echo edit RYUJINX at the top of this script
    exit /b 1
)

if not exist "%NRO%" (
    echo nro not found at %NRO%
    echo build it first: build_switch.bat
    exit /b 1
)

rem Everything the emulator writes goes to a file as well as to the console. A guest crash prints
rem its stack trace and register dump to stderr and nowhere else - Ryujinx keeps no file log of its
rem own by default - so without this the only copy of a crash scrolls past and is gone with the
rem window. Both streams are merged, because the interesting lines are split across them.
set LOG_DIRECTORY=%~dp0logs
if not exist "%LOG_DIRECTORY%" mkdir "%LOG_DIRECTORY%"

for /f "delims=" %%t in ('powershell -NoProfile -Command "Get-Date -Format yyyy-MM-dd__HH-mm-ss"') do set STAMP=%%t
set LOG_FILE=%LOG_DIRECTORY%\ryujinx_%STAMP%.txt

echo launching %NRO%
echo emulator output: %LOG_FILE%
echo.

rem Plain redirection rather than a tee: powershell wraps a native program's stderr
rem as NativeCommandError noise and Tee-Object writes utf-16, which findstr cannot
rem search. The console stays quiet during the run; the file has everything.
"%RYUJINX%" "%NRO%" > "%LOG_FILE%" 2>&1

rem A run that ends in a guest crash looks like a normal exit from the outside, so say so plainly
rem rather than leaving it to be spotted in the scrollback.
findstr /C:"InvalidAccessHandler" /C:"PrintGuestStackTrace" "%LOG_FILE%" >nul 2>&1
if not errorlevel 1 (
    echo.
    echo *** THE GUEST CRASHED - stack trace and registers are in:
    echo ***   %LOG_FILE%
    echo.
    findstr /C:"InvalidAccessHandler" "%LOG_FILE%"
)

rem The guest writes its own log to the emulated sd card, and on real hardware that log is the
rem only artefact a run leaves behind, so it is worth knowing where it lands.
echo.
echo guest log directory: %APPDATA%\Ryujinx\sdcard\switch\deceptus\logs
for /f "delims=" %%f in ('dir /b /o-d "%APPDATA%\Ryujinx\sdcard\switch\deceptus\logs\*.log" 2^>nul') do (
    echo newest log: %APPDATA%\Ryujinx\sdcard\switch\deceptus\logs\%%f
    goto :done
)
:done
