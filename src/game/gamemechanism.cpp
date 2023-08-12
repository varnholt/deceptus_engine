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

AudioUpdateBehavior GameMechanism::getAudioUpdateBehavior() const
{
   return _audio_update_behavior;
}

void GameMechanism::setAudioUpdateBehavior(AudioUpdateBehavior newAudio_update_behavior)
{
   _audio_update_behavior = newAudio_update_behavior;
}

std::optional<int32_t> GameMechanism::getRoomId() const
{
   return _room_id;
}

void GameMechanism::setRoomId(int32_t room_id)
{
   _room_id = room_id;
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

void GameMechanism::addChunks(const sf::FloatRect& bouding_box)
{
   _chunks.push_back({bouding_box.left, bouding_box.top});
   _chunks.push_back({bouding_box.left, bouding_box.top + bouding_box.height});
   _chunks.push_back({bouding_box.left + bouding_box.width, bouding_box.top + bouding_box.height});
   _chunks.push_back({bouding_box.left + bouding_box.width, bouding_box.top});
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
