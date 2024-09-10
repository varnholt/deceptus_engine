#include "gamemechanism.h"
#include "game/mechanisms/gamemechanismobserver.h"

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

float GameMechanism::getReferenceVolume() const
{
   return _reference_volume;
}

AudioUpdateBehavior GameMechanism::getAudioUpdateBehavior() const
{
   return _audio_update_data._update_behavior;
}

void GameMechanism::setAudioUpdateBehavior(AudioUpdateBehavior audio_update_behavior)
{
   _audio_update_data._update_behavior = audio_update_behavior;
}

const std::vector<int32_t>& GameMechanism::getRoomIds() const
{
   return _audio_update_data._room_ids;
}

void GameMechanism::setRoomIds(const std::vector<int32_t>& room_ids)
{
   _audio_update_data._room_ids = room_ids;
}

void GameMechanism::addRoomId(int32_t room_id)
{
   _audio_update_data._room_ids.push_back(room_id);
}

void GameMechanism::setReferenceVolume(float volume)
{
   _reference_volume = volume;
}

void GameMechanism::setVolume(float volume)
{
   _audio_update_data._volume = volume;
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
   _chunks.emplace_back(bouding_box.left, bouding_box.top);
   _chunks.emplace_back(bouding_box.left, bouding_box.top + bouding_box.height);
   _chunks.emplace_back(bouding_box.left + bouding_box.width, bouding_box.top + bouding_box.height);
   _chunks.emplace_back(bouding_box.left + bouding_box.width, bouding_box.top);
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
   const auto changed = _enabled != enabled;
   _enabled = enabled;

   if (_observed && changed)
   {
      GameMechanismObserver::onEnabled(enabled);
   }
}

void GameMechanism::toggle()
{
   setEnabled(!isEnabled());
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
   return _audio_update_data._range;
}
