#ifndef PLAYERAUDIO_H
#define PLAYERAUDIO_H

#include <SFML/Graphics.hpp>

namespace PlayerAudio
{
/// \brief registers all player-specific audio samples in the global audio system.
void addSamples();
/// \brief sets the sfml listener to the player's current world position.
/// \param pos listener position in world coordinates.
void updateListenerPosition(sf::Vector2f pos);
};  // namespace PlayerAudio

#endif  // PLAYERAUDIO_H
