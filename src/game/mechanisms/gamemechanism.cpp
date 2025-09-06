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

bool GameMechanism::isDestructible() const
{
   return false;
}

const std::vector<Hitbox>& GameMechanism::getHitboxes()
{
   static std::vector<Hitbox> empty;
   return empty;
}

void GameMechanism::hit(int32_t damage)
{
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

void GameMechanism::addChunks(const sf::FloatRect& bounding_box)
{
   constexpr int32_t chunk_size_x = 1 << CHUNK_SHIFT_X;  // 512
   constexpr int32_t chunk_size_y = 1 << CHUNK_SHIFT_Y;  // 512
   for (auto y = static_cast<int32_t>(bounding_box.position.y); y <= static_cast<int32_t>(bounding_box.position.y + bounding_box.size.y);
        y += chunk_size_y)
   {
      for (auto x = static_cast<int32_t>(bounding_box.position.x); x <= static_cast<int32_t>(bounding_box.position.x + bounding_box.size.x);
           x += chunk_size_x)
      {
         Chunk chunk(x, y);
         if (!std::ranges::any_of(_chunks, [&chunk](const Chunk& existing_chunk) { return existing_chunk == chunk; }))
         {
            _chunks.push_back(chunk);
         }
      }
   }
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
      GameMechanismObserver::onEnabled("todo", "todo", enabled);
   }
}

void GameMechanism::toggle()
{
   setEnabled(!isEnabled());
}

bool GameMechanism::isVisible() const
{
   return _visible;
}

void GameMechanism::setVisible(bool visible)
{
   _visible = visible;
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
