#pragma once

#include <SFML/Graphics.hpp>
#include <chrono>
#include <thread>

namespace SplashScreen
{
/// \brief shows the startup splash image with fade-in and fade-out animation.
/// \param window render window used to display the splash screen.
void show(sf::RenderWindow& window);
};
