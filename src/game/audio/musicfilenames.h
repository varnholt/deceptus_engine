#ifndef MUSICFILENAMES_H
#define MUSICFILENAMES_H

#include <filesystem>

namespace MusicFilenames
{
std::filesystem::path getLevelMusic();
std::filesystem::path getMenuMusic();
void setLevelMusic(const std::filesystem::path& music);
void setMenuMusic(const std::filesystem::path& music);
};

#endif  // MUSICFILENAMES_H
