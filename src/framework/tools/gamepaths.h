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
/// \brief Resolves a writable preferences file inside the settings directory.
///
/// On first access the file is seeded from the bundled default in data/config when it does not
/// exist yet, which also migrates existing progress written by earlier versions. Reads and writes
/// should both use the returned path so data/config stays a read-only default source.
///
/// \param filename bare file name, e.g. "savestate.json".
/// \return Absolute path to the writable preferences file.
///
std::filesystem::path getPreferencesFile(const std::string& filename);

///
/// \brief Flushes pending writes to persistent storage.
///
/// No-op on desktop. On the web build this triggers an IDBFS sync so saves survive a page reload.
///
void flushToPersistentStorage();

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
