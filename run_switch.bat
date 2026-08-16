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

echo launching %NRO%
"%RYUJINX%" "%NRO%"

rem The guest writes its own log to the emulated sd card, and on real hardware that log is the
rem only artefact a run leaves behind, so it is worth knowing where it lands.
echo.
echo guest log directory: %APPDATA%\Ryujinx\sdcard\switch\deceptus\logs
for /f "delims=" %%f in ('dir /b /o-d "%APPDATA%\Ryujinx\sdcard\switch\deceptus\logs\*.log" 2^>nul') do (
    echo newest log: %APPDATA%\Ryujinx\sdcard\switch\deceptus\logs\%%f
    goto :done
)
:done
