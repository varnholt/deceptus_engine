#ifndef INGAMEMENUPAGE_H
#define INGAMEMENUPAGE_H

#include "framework/image/layer.h"

#include <SFML/Graphics.hpp>
#include <chrono>
#include <map>
#include <optional>
#include <ostream>
#include <sstream>

/// \brief base class for layered in-game menu pages with shared transition behavior.
class InGameMenuPage
{
public:
   /// \brief identifies the active transition animation for a menu page.
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

   /// \brief constructs an empty menu page before PSD layers are loaded.
   InGameMenuPage() = default;

   /// \brief destroys the page through a polymorphic base pointer.
   virtual ~InGameMenuPage() = default;

   /// \brief draws all visible PSD-backed layers of the page.
   /// \param window render target that receives the page layers.
   /// \param states render states used for drawing.
   virtual void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates{});

   /// \brief advances page-specific state and animations.
   /// \param dt elapsed frame time passed by the menu manager.
   virtual void update(const sf::Time& dt) = 0;

   /// \brief starts the page show transition and performs an immediate update step.
   virtual void show();

   /// \brief starts the page hide transition unless another animation is active.
   virtual void hide();

   /// \brief finalizes hiding by resuming gameplay and clearing ingame menu display mode.
   virtual void fullyHidden();

   /// \brief handles a left navigation action.
   virtual void left() {};

   /// \brief handles a right navigation action.
   virtual void right() {};

   /// \brief handles an up navigation action.
   virtual void up() {};

   /// \brief handles a down navigation action.
   virtual void down() {};

   /// \brief handles a keyboard key press forwarded from the menu container.
   /// \param key pressed keyboard key to interpret.
   virtual void keyboardKeyPressed(sf::Keyboard::Key /*key*/){};

   /// \brief starts a transition that slides the page out to the left.
   void moveOutToLeft();

   /// \brief starts a transition that slides the page in from the left.
   void moveInFromLeft();

   /// \brief starts a transition that slides the page out to the right.
   void moveOutToRight();

   /// \brief starts a transition that slides the page in from the right.
   void moveInFromRight();

   /// \brief returns the currently active animation, if any.
   /// \return active animation state or std::nullopt when idle.
   std::optional<Animation> getAnimation() const;

protected:
   /// \brief loads layers from the configured PSD file into drawable page structures.
   void load();

   /// \brief computes the current horizontal offset for move animations.
   /// \return current x offset in pixels, or std::nullopt when move animation finished.
   std::optional<float> getMoveOffset() const;

   /// \brief hook for page-specific debugging output.
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

/// \brief writes a readable animation enum name to an output stream.
/// \param os destination stream.
/// \param animation animation value to format.
/// \return output stream after writing the animation name.
std::ostream& operator<<(std::ostream& os, InGameMenuPage::Animation animation);

#endif  // INGAMEMENUPAGE_H
