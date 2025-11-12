#include "musicfilenames.h"

namespace
{
std::filesystem::path _level_music{"data/music/level_test_track_muffler_awakening.ogg"};
std::filesystem::path _menu_music{"data/music/menu_test_track_muffler_callisto.ogg"};
}  // namespace

std::filesystem::path MusicFilenames::getLevelMusic()
{
   return _level_music;
}

std::filesystem::path MusicFilenames::getMenuMusic()
{
   return _menu_music;
}

void MusicFilenames::setLevelMusic(const std::filesystem::path& music)
{
   _level_music = music;
}

void MusicFilenames::setMenuMusic(const std::filesystem::path& music)
{
   _menu_music = music;
}
