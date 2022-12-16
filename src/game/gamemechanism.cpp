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

bool GameMechanism::hasAudio() const
{
   return false;
}

std::optional<AudioRange> GameMechanism::getAudioRange() const
{
   return std::nullopt;
}
