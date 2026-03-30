#ifndef BLENDMODEDESERIALIZER_H
#define BLENDMODEDESERIALIZER_H

#include <map>
#include <memory>
#include <optional>
#include "SFML/Graphics.hpp"

struct TmxProperty;

namespace BlendModeDeserializer
{
/// \brief deserializes an SFML blend mode from TMX property values.
/// \param map property map read from a TMX object or layer.
/// \return resolved blend mode, or std::nullopt when no valid blend mode properties are present.
std::optional<sf::BlendMode> readBlendMode(const std::map<std::string, std::shared_ptr<TmxProperty>>& map);
};

#endif  // BLENDMODEDESERIALIZER_H
