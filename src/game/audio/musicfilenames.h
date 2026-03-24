#ifndef MUSICFILENAMES_H
#define MUSICFILENAMES_H

#include <filesystem>

namespace MusicFilenames
{
/// \brief returns the configured default music file path for gameplay levels.
/// \return filesystem path to the current level music track.
std::filesystem::path getLevelMusic();
/// \brief returns the configured default music file path for menu screens.
/// \return filesystem path to the current menu music track.
std::filesystem::path getMenuMusic();
/// \brief overrides the stored default gameplay level music path.
/// \param music new filesystem path for the level music track.
void setLevelMusic(const std::filesystem::path& music);
/// \brief overrides the stored default menu music path.
/// \param music new filesystem path for the menu music track.
void setMenuMusic(const std::filesystem::path& music);
};

#endif  // MUSICFILENAMES_H
