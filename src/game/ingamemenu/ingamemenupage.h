#ifndef INGAMEMENUPAGE_H
#define INGAMEMENUPAGE_H

#include "framework/image/layer.h"

#include <SFML/Graphics.hpp>
#include <chrono>
#include <map>
#include <optional>
#include <ostream>
#include <sstream>

class InGameMenuPage
{
public:
   enum class Animation
   {
      Show,
      Hide,
      MoveInFromLeft,
      MoveOutToLeft,
      MoveInFromRight,
      MoveOutToRight,
   };

   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;
   using FloatSeconds = std::chrono::duration<float>;

   InGameMenuPage() = default;
   virtual ~InGameMenuPage() = default;

   virtual void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);
   virtual void update(const sf::Time& dt) = 0;

   virtual void show();
   virtual void hide();

   virtual void fullyHidden();

   virtual void left() {};
   virtual void right() {};
   virtual void up() {};
   virtual void down() {};

   void moveOutToLeft();
   void moveInFromLeft();
   void moveOutToRight();
   void moveInFromRight();

   std::optional<Animation> getAnimation() const;

protected:
   void load();
   std::optional<float> getMoveOffset() const;

   void debug();

   std::string _filename;

   std::vector<std::shared_ptr<Layer>> _layer_stack;
   std::map<std::string, std::shared_ptr<Layer>> _layers;

   // animation
   HighResTimePoint _time_show;
   HighResTimePoint _time_hide;
   HighResTimePoint _time_move;
   std::optional<Animation> _animation;
   float _move_offset = 0.0f;

   friend std::ostream& operator<<(std::ostream& os, Animation dt);
};

std::ostream& operator<<(std::ostream& os, InGameMenuPage::Animation animation);

#endif  // INGAMEMENUPAGE_H
