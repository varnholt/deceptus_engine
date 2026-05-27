#pragma once

#include "menuscreen.h"

#include <memory>

/// \brief credits screen that exposes back navigation and device-specific prompts.
class MenuScreenCredits : public MenuScreen
{
public:
   /// \brief initializes the credits screen with its PSD layout and credit text.
   MenuScreenCredits();

   /// \brief draws the credits screen layers and credit text entries.
   /// \param window render target receiving the credits screen.
   /// \param states render states forwarded to draw calls.
   void draw(sf::RenderTarget& window, sf::RenderStates states) override;

   /// \brief refreshes prompt visibility after PSD layers are loaded.
   void loadingFinished() override;

   /// \brief updates controller or keyboard prompts for leaving the credits screen.
   void updateLayers();

   /// \brief processes upward navigation input and plays navigation feedback.
   void up();

   /// \brief processes downward navigation input and plays navigation feedback.
   void down();

   /// \brief processes confirm input and plays selection feedback.
   void select();

   /// \brief returns to the options menu.
   void back();

   /// \brief routes navigation and cancel keys to credits screen actions.
   /// \param key key that was pressed.
   void keyboardKeyPressed(sf::Keyboard::Key key) override;

private:
   sf::Font _font;
   std::unique_ptr<sf::Text> _text_code;
   std::unique_ptr<sf::Text> _text_artwork;
};
