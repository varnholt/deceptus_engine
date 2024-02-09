#ifndef BLENDMODEDESERIALIZER_H
#define BLENDMODEDESERIALIZER_H

#include <map>
#include <memory>
#include <optional>
#include "SFML/Graphics.hpp"

struct TmxProperty;

namespace BlendModeDeserializer
{
std::optional<sf::BlendMode> readBlendMode(const std::map<std::string, std::shared_ptr<TmxProperty>>& map);
};

#endif  // BLENDMODEDESERIALIZER_H
