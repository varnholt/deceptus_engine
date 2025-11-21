#pragma once

#include "menuscreen.h"

#include <cstdint>
#include <functional>

class MenuScreenMain : public MenuScreen
{
public:
   enum class Selection
   {
      Start,
      Options,
      Quit
   };

   using ExitCallback = std::function<void(void)>;

   MenuScreenMain();

   void update(const sf::Time& dt) override;
   void draw(sf::RenderTarget& window, sf::RenderStates states) override;

   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   void loadingFinished() override;
   void updateLayers();
   void showEvent() override;  //<! reset fade when menu is shown

   void up();
   void down();
   void select();

   void setExitCallback(ExitCallback callback);

   Selection _selection = Selection::Start;

private:
   ExitCallback _exit_callback;

   sf::Font _font;
   std::unique_ptr<sf::Text> _text_build;
   std::unique_ptr<sf::Text> _text_year;

   // fade-in
   bool _fade_in_active = false;   //<! set to true only when first shown
   bool _first_time_shown = true;  //<! track if this is the first time the menu is shown
   sf::Clock _fade_in_clock;
   static constexpr float _fade_in_duration = 1000.0f;  // 1 second fade in
   uint8_t _fade_alpha{0};
};
