#pragma once

#include "menu.h"
#include "menuscreen.h"

/// \brief options hub screen that links to controls, video, audio, game, achievements, and credits.
class MenuScreenOptions : public MenuScreen
{
public:
   /// \brief initializes the options screen with its PSD layout.
   MenuScreenOptions();

   /// \brief identifies selectable option categories.
   enum class Selection
   {
      Controls = 0,
      Video = 1,
      Audio = 2,
      Game = 3,
      Achievements = 4,
      Credits = 5,
      Count
   };

   /// \brief routes navigation, confirm, and cancel keys to options screen actions.
   /// \param key key that was pressed.
   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   /// \brief updates option highlights and input prompts.
   void updateLayers();

   /// \brief refreshes layers after PSD loading.
   void loadingFinished() override;

   /// \brief moves selection up, skipping achievements, with wrap-around.
   void up();

   /// \brief moves selection down, skipping achievements, with wrap-around.
   void down();

   /// \brief returns to the most recent parent menu that opened options.
   void back();

   /// \brief captures the back-navigation target when the screen becomes active.
   void showEvent() override;

   /// \brief draws the options screen layers and item text.
   /// \param window render target receiving the options screen.
   /// \param states render states forwarded to draw calls.
   void draw(sf::RenderTarget& window, sf::RenderStates states) override;

   /// \brief opens the submenu corresponding to the current selection.
   void select();

   Selection _selection = Selection::Controls;

private:
   Menu::MenuType _back_target = Menu::MenuType::Main;  //!< parent menu to return to when pressing back.

   std::unique_ptr<sf::Text> _text_back_button;
   std::unique_ptr<sf::Text> _text_accept_button;

   std::unique_ptr<sf::Text> _text_controls_item;
   std::unique_ptr<sf::Text> _text_video_item;
   std::unique_ptr<sf::Text> _text_audio_item;
   std::unique_ptr<sf::Text> _text_game_item;
   std::unique_ptr<sf::Text> _text_achievements_item;
   std::unique_ptr<sf::Text> _text_credits_item;
};
