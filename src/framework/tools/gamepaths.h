#pragma once

#include <filesystem>
#include <string>

///
/// \brief Resolves and creates the application's data directories.
///
namespace GamePaths
{
///
/// \brief Returns the platform-specific base directory for deceptus data.
/// \return Base data directory path.
///
std::filesystem::path getGameDataDir();

///
/// \brief Returns the settings directory and creates it when missing.
/// \return Settings directory path.
///
std::filesystem::path getSettingsDir();

///
/// \brief Returns the log directory and creates it when missing.
/// \return Log directory path.
///
std::filesystem::path getLogDir();

///
/// \brief Returns the recordings directory and creates it when missing.
/// \return Recordings directory path.
///
std::filesystem::path getRecordingDir();

///
/// \brief Creates the standard settings, logs, and recordings directories.
///
void createGameDirectories();

}  // namespace GamePaths
