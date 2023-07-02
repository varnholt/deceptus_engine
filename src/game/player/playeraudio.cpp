#include "playeraudio.h"

#include "game/audio.h"

#include <SFML/Audio.hpp>

void PlayerAudio::updateListenerPosition(sf::Vector2f pos)
{
   // The default listener's up vector is (0, 1, 0)
   // sf::Listener::setUpVector
   sf::Listener::setPosition(pos.x, pos.y, 0.0f);
}

void PlayerAudio::addSamples()
{
   auto& audio = Audio::getInstance();

   audio.addSample("player_dash_01.wav");
   audio.addSample("player_doublejump_01.mp3");
   audio.addSample("player_grunt_01.wav");
   audio.addSample("player_grunt_02.wav");
   audio.addSample("player_jump_land.wav");
   audio.addSample("player_jump_liftoff.wav");
   audio.addSample("player_kneel_01.wav");
   audio.addSample("player_footstep_stone_l.wav");
   audio.addSample("player_footstep_stone_r.wav");
   audio.addSample("player_spawn_01.wav");
   audio.addSample("player_sword_kneeling_01.wav");
   audio.addSample("player_sword_kneeling_02.wav");
   audio.addSample("player_sword_kneeling_03.wav");
   audio.addSample("player_sword_kneeling_04.wav");
   audio.addSample("player_sword_standing_01.wav");
   audio.addSample("player_sword_standing_02.wav");
   audio.addSample("player_sword_standing_03.wav");
   audio.addSample("player_sword_standing_04.wav");
   audio.addSample("player_sword_standing_05.wav");
   audio.addSample("player_sword_standing_06.wav");
   audio.addSample("player_sword_standing_07.wav");
   audio.addSample("player_sword_standing_08.wav");
   audio.addSample("player_sword_standing_09.wav");
   audio.addSample("player_wallslide_01.wav");
}
