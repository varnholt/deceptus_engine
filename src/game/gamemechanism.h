#pragma once

#include "game/audiorange.h"
#include "game/chunk.h"
#include "game/constants.h"

#include "SFML/Graphics.hpp"
#include "json/json.hpp"

#include <cstdint>
#include <optional>

struct Room;

class GameMechanism
{
public:
   GameMechanism() = default;
   virtual ~GameMechanism() = default;

   virtual void draw(sf::RenderTarget& target, sf::RenderTarget& normal);
   virtual void update(const sf::Time& dt);

   virtual bool isEnabled() const;
   virtual void setEnabled(bool enabled);

   virtual void preload();

   // audio related
   virtual bool hasAudio() const;
   virtual std::optional<AudioRange> getAudioRange() const;
   virtual bool isAudioEnabled() const;
   virtual void setAudioEnabled(bool audio_enabled);
   virtual void setVolume(float volume);

   virtual bool hasChunks() const;
   virtual const std::vector<Chunk>& getChunks() const;
   virtual void addChunks(const sf::FloatRect& bounding_box);

   virtual int32_t getZ() const;
   virtual void setZ(const int32_t& z);

   virtual std::optional<sf::FloatRect> getBoundingBoxPx() = 0;

   virtual std::optional<int32_t> getRoomId() const;
   virtual void setRoomId(int32_t room_id);

   virtual void serializeState(nlohmann::json&);
   virtual void deserializeState(const nlohmann::json&);
   virtual bool isSerialized() const;

   virtual AudioUpdateBehavior getAudioUpdateBehavior() const;
   virtual void setAudioUpdateBehavior(AudioUpdateBehavior newAudio_update_behavior);

protected:
   int32_t _z_index{0};
   bool _enabled{true};
   bool _serialized{false};
   std::optional<int32_t> _room_id;

   // audio related
   bool _has_audio{false};
   bool _audio_enabled{false};
   float _volume{0.0f};
   std::optional<AudioRange> _audio_range;
   AudioUpdateBehavior _audio_update_behavior{AudioUpdateBehavior::RangeBased};

   std::vector<Chunk> _chunks;
   MechanismVersion _version = MechanismVersion::Version1;
};
