#pragma once

#include "audiorange.h"
#include "chunk.h"
#include "constants.h"

#include "SFML/Graphics.hpp"
#include "json/json.hpp"

#include <cstdint>
#include <optional>

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
   virtual void setAudioEnabled(bool newAudio_enabled);
   virtual void setVolume(float volume);

   virtual bool hasChunks() const;
   virtual const std::vector<Chunk>& getChunks() const;
   virtual void addChunks(const sf::FloatRect& bounding_box);

   virtual int32_t getZ() const;
   virtual void setZ(const int32_t& z);

   virtual std::optional<sf::FloatRect> getBoundingBoxPx() = 0;

   virtual void serializeState(nlohmann::json&);
   virtual void deserializeState(const nlohmann::json&);
   virtual bool isSerialized() const;

protected:
   int32_t _z_index{0};
   bool _enabled{true};
   bool _serialized{false};
   bool _has_audio{false};
   bool _audio_enabled{false};
   float _volume{0.0f};
   std::optional<AudioRange> _audio_range;
   std::vector<Chunk> _chunks;
   MechanismVersion _version = MechanismVersion::Version1;
};
