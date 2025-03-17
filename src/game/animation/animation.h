#pragma once

#include <SFML/Graphics.hpp>

#include "game/constants.h"

#include <chrono>
#include <memory>
#include <vector>

class Animation : public sf::Drawable, public sf::Transformable
{
public:
   using HighResDuration = std::chrono::high_resolution_clock::duration;

   Animation() = default;
   Animation(const Animation& anim);

   void draw(sf::RenderTarget& target, sf::RenderStates states = {}) const override;
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal, sf::RenderStates states = {}) const;
   void drawTree(sf::RenderTarget& target, sf::RenderStates states = {}) const;
   void drawTree(sf::RenderTarget& color, sf::RenderTarget& normal, sf::RenderStates states = {}) const;

   void update(const sf::Time& dt);
   void play();
   void pause();
   void stop();
   void seekToStart();

   void updateTree(const sf::Time& dt);
   void playTree();
   void pauseTree();
   void stopTree();
   void seekToStartTree();

   void updateVertices(bool reset_time = true);

   void setColor(const sf::Color& color);
   void setColorTree(const sf::Color& color);
   void setAlpha(uint8_t alpha);
   void setAlphaTree(uint8_t alpha);
   bool isVisible() const;
   void setVisible(bool newVisible);

   sf::FloatRect getLocalBounds() const;
   sf::FloatRect getGlobalBounds() const;

   void setFrameTimes(const std::vector<sf::Time>& frame_times);
   size_t getFrameCount() const;
   void reverse();

   void addChild(const std::shared_ptr<Animation>& child);

   std::string _name;
   std::vector<sf::IntRect> _frames;
   std::shared_ptr<sf::Texture> _color_texture;
   std::shared_ptr<sf::Texture> _normal_texture;
   sf::Vertex _vertices[4];

   sf::Time _current_time;
   sf::Time _elapsed;
   sf::Time _overall_time;
   HighResDuration _overall_time_chrono;
   int32_t _current_frame{0};
   int32_t _previous_frame{-1};
   int32_t _loop_count{0};

   bool _paused{false};
   bool _looped{false};
   bool _reset_to_first_frame{true};
   bool _finished{false};
   bool _visible{true};

   std::vector<std::shared_ptr<Animation>> _children;

   const std::vector<sf::Time>& getFrameTimes() const;

private:
   std::vector<sf::Time> _frame_times;
};
