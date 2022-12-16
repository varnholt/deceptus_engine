#pragma once

#include "audiorange.h"
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

   virtual bool hasAudio() const;
   virtual std::optional<AudioRange> getAudioRange() const;

   virtual int32_t getZ() const;
   virtual void setZ(const int32_t& z);

   virtual std::optional<sf::FloatRect> getBoundingBoxPx() = 0;

   virtual void serializeState(nlohmann::json&);
   virtual void deserializeState(const nlohmann::json&);
   virtual bool isSerialized() const;

protected:
   int32_t _z_index = 0;
   bool _enabled = true;
   bool _serialized = false;
   MechanismVersion _version = MechanismVersion::Version1;
};
