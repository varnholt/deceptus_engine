#include "playeraudio.h"

#include <SFML/Audio.hpp>

void PlayerAudio::updateListenerPosition(sf::Vector2f pos)
{
   // The default listener's up vector is (0, 1, 0)
   // sf::Listener::setUpVector
   sf::Listener::setPosition(pos.x, pos.y, 0.0f);
}
