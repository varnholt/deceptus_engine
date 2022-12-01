#ifndef INGAMEMENUPAGE_H
#define INGAMEMENUPAGE_H

#include "framework/image/layer.h"

#include <chrono>
#include <SFML/Graphics.hpp>

class InGameMenuPage
{
public:
   enum class Animation
   {
      Show,
      Hide,
      MoveLeft,
      MoveRight,
   };

   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;
   using FloatSeconds = std::chrono::duration<float>;

   InGameMenuPage() = default;

   virtual void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);
   virtual void update(const sf::Time& dt) = 0;

   virtual void show() = 0;
   virtual void hide() = 0;

   void moveOutLeft();
   void moveInLeft();
   void moveOutRight();
   void moveInRight();

   std::optional<Animation> getAnimation() const;

protected:
   void load();
   std::optional<float> getMoveOffset() const;

   std::string _filename;

   std::vector<std::shared_ptr<Layer>> _layer_stack;
   std::map<std::string, std::shared_ptr<Layer>> _layers;

   sf::Font _font;
   sf::Text _text;

   // animation
   HighResTimePoint _time_show;
   HighResTimePoint _time_hide;
   HighResTimePoint _time_move;
   std::optional<Animation> _animation;
   float _move_offset = 0.0f;
};

#endif // INGAMEMENUPAGE_H
