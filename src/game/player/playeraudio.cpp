#include "playeraudio.h"

#include "game/audio/audio.h"

#include <SFML/Audio.hpp>

void PlayerAudio::updateListenerPosition(sf::Vector2f pos)
{
   Audio::getInstance().updateListenerPosition(pos);
}

void PlayerAudio::addSamples()
{
   auto& audio = Audio::getInstance();

   audio.addSample("player_dash_01.ogg");
   audio.addSample("player_doublejump_01.ogg");
   audio.addSample("player_grunt_01.ogg");
   audio.addSample("player_grunt_02.ogg");
   audio.addSample("player_jump_land.ogg");
   audio.addSample("player_jump_liftoff.ogg");
   audio.addSample("player_kneel_01.ogg");
   audio.addSample("player_footstep_stone_l.ogg");
   audio.addSample("player_footstep_stone_r.ogg");
   audio.addSample("player_spawn_01.ogg");
   audio.addSample("player_sword_kneeling_01.ogg");
   audio.addSample("player_sword_kneeling_02.ogg");
   audio.addSample("player_sword_kneeling_03.ogg");
   audio.addSample("player_sword_kneeling_04.ogg");
   audio.addSample("player_sword_standing_01.ogg");
   audio.addSample("player_sword_standing_02.ogg");
   audio.addSample("player_sword_standing_03.ogg");
   audio.addSample("player_sword_standing_04.ogg");
   audio.addSample("player_sword_standing_05.ogg");
   audio.addSample("player_sword_standing_06.ogg");
   audio.addSample("player_sword_standing_07.ogg");
   audio.addSample("player_sword_standing_08.ogg");
   audio.addSample("player_sword_standing_09.ogg");
   audio.addSample("player_wallslide_01.ogg");
}
