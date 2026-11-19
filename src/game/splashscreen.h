#pragma once

#include <SFML/Graphics.hpp>
#include <string>

namespace SplashScreen
{
/// \brief plays the configured startup animation and its soundtrack once per process.
/// \param window borrowed render window with display settings already applied.
/// \param config_path animation JSON path resolved through the asset source.
/// \details releases textures and stops the soundtrack on return; invalid configuration is logged and startup continues.
void show(sf::RenderWindow& window, const std::string& config_path);
}  // namespace SplashScreen
