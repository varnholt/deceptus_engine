#include "gamemechanism.h"

int32_t GameMechanism::getZ() const
{
   return _z_index;
}

void GameMechanism::setZ(const int32_t& z)
{
   _z_index = z;
}

void GameMechanism::serializeState(nlohmann::json&)
{
}

void GameMechanism::deserializeState(const nlohmann::json&)
{
}

bool GameMechanism::isSerialized() const
{
   return _serialized;
}

void GameMechanism::setVolume(float volume)
{
   _volume = volume;
}

bool GameMechanism::hasChunks() const
{
   return !_chunks.empty();
}

const std::vector<Chunk>& GameMechanism::getChunks() const
{
   return _chunks;
}

bool GameMechanism::isAudioEnabled() const
{
   return _audio_enabled;
}

void GameMechanism::setAudioEnabled(bool audio_enabled)
{
   _audio_enabled = audio_enabled;
}

void GameMechanism::draw(sf::RenderTarget& /*target*/, sf::RenderTarget& /*normal*/)
{
}

void GameMechanism::update(const sf::Time& /*dt*/)
{
}

bool GameMechanism::isEnabled() const
{
   return _enabled;
}

void GameMechanism::setEnabled(bool enabled)
{
   _enabled = enabled;
}

void GameMechanism::preload()
{
}

bool GameMechanism::hasAudio() const
{
   return _has_audio;
}

std::optional<AudioRange> GameMechanism::getAudioRange() const
{
   return _audio_range;
}
