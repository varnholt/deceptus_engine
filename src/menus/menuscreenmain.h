#pragma once

#include "menuscreen.h"

#include <cstdint>
#include <functional>

/// \brief title screen with start/options/quit selection and one-time fade-in rendering.
class MenuScreenMain : public MenuScreen
{
public:
   /// \brief identifies selectable rows on the title screen.
   enum class Selection
   {
      Start,
      Options,
      Quit
   };

   using ExitCallback = std::function<void(void)>;

   /// \brief initializes title-screen assets, build text, and copyright year text.
   MenuScreenMain();

   /// \brief runs first-show fade-in timing and alpha interpolation.
   /// \param dt frame delta time.
   void update(const sf::Time& dt) override;

   /// \brief draws title-screen layers, optionally through a temporary texture during fade-in.
   /// \param window render target receiving the title screen.
   /// \param states render states forwarded to draw calls.
   void draw(sf::RenderTarget& window, sf::RenderStates states) override;

   /// \brief routes up, down, and enter keys to title-screen navigation and selection.
   /// \param key key that was pressed.
   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   /// \brief reloads save metadata and updates layer visibility once PSD loading completes.
   void loadingFinished() override;

   /// \brief updates highlighted menu rows and start-or-continue labels.
   void updateLayers();

   /// \brief moves selection one row up with wrap-around.
   void up();

   /// \brief moves selection one row down with wrap-around.
   void down();

   /// \brief enters the selected title-screen action.
   void select();

   /// \brief sets the callback invoked when quit is confirmed.
   /// \param callback function called after the quit confirmation dialog returns yes.
   void setExitCallback(ExitCallback callback);

   Selection _selection = Selection::Start;

private:
   ExitCallback _exit_callback;

   sf::Font _font;
   std::unique_ptr<sf::Text> _text_build;
   std::unique_ptr<sf::Text> _text_year;

   bool _fade_in_active = false;
   bool _first_time_shown = true;
   sf::Clock _fade_in_clock;
   uint8_t _fade_alpha{0};
};
