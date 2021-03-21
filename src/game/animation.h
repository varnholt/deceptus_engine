#pragma once

#include <SFML/Graphics.hpp>

#include "game/constants.h"

#include <memory>
#include <vector>


class Animation : public sf::Sprite
{

public:

   Animation() = default;

   void draw(sf::RenderTarget& target, sf::RenderStates states = {}) const override;
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal, sf::RenderStates states = {}) const;

   void update(const sf::Time& dt);

   void play();
   void pause();
   void stop();
   void seekToStart();
   void setFrame(int32_t newFrame, bool resetTime = true);

   void setAlpha(uint8_t alpha);

   sf::FloatRect getLocalBounds() const;
   sf::FloatRect getGlobalBounds() const;

   std::string _name;

   std::vector<sf::IntRect> _fames;

   std::shared_ptr<sf::Texture> _texture_map;
   std::shared_ptr<sf::Texture> _normal_map;

   sf::Vertex _vertices[4];

   sf::Time _current_time;
   sf::Time _elapsed;
   sf::Time _overall_time;

   int32_t _current_frame = 0;
   int32_t _previous_frame = -1;

   bool _paused = false;
   bool _looped = false;
   bool _reset_to_first_frame = true;

   void setFrameTimes(const std::vector<sf::Time>& frameTimes);

private:
   std::vector<sf::Time> _frame_times;
};

