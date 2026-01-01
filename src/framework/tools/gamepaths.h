#pragma once

#include <filesystem>
#include <string>

/*!
 * GamePaths namespace provides functions to get standard directories for the game
 * such as settings, logs, recordings, etc. It follows platform conventions for
 * where to store game data.
 *
 * The directory structure should be initialized once at application startup
 * by calling createGameDirectories() from main.cpp.
 */
namespace GamePaths
{

/*!
 * Gets the base directory for game data.
 * On Windows: %APPDATA%\deceptus
 * On Linux/macOS: ~/.local/share/deceptus
 */
std::filesystem::path getGameDataDir();

/*!
 * Gets the directory for game settings/config files.
 */
std::filesystem::path getSettingsDir();

/*!
 * Gets the directory for game log files.
 */
std::filesystem::path getLogDir();

/*!
 * Gets the directory for game recording files.
 */
std::filesystem::path getRecordingDir();

/*!
 * Creates the directory structure for the game if it doesn't exist.
 * This includes all the standard directories like settings, logs, recordings, etc.
 */
void createGameDirectories();

} // namespace GamePaths
