#pragma once

#include "menuscreen.h"

/// \brief gameplay settings screen for text speed, rumble, and automatic pause behavior.
class MenuScreenGame : public MenuScreen
{
public:
   /// \brief identifies selectable gameplay settings rows.
   enum class Selection
   {
      TextSpeed = 0,
      Rumble = 1,
      AutomaticPause = 2,
      Count = 3,
   };

   /// \brief initializes the gameplay settings screen with its PSD layout.
   MenuScreenGame();

protected:
   /// \brief routes navigation and adjustment keys to gameplay settings actions.
   /// \param key key that was pressed.
   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   /// \brief refreshes screen layers after PSD loading.
   void loadingFinished() override;

private:
   /// \brief returns whether rumble is enabled in the persisted game configuration.
   /// \return true when rumble is enabled.
   bool isRumbleEnabled() const;

   /// \brief updates highlights, prompts, and value indicators for gameplay settings.
   void updateLayers();

   /// \brief moves selection to the previous settings row with wrap-around.
   void up();

   /// \brief moves selection to the next settings row with wrap-around.
   void down();

   /// \brief confirms the current row and plays apply feedback.
   void select();

   /// \brief returns to the options menu.
   void back();

   /// \brief modifies the currently selected gameplay setting and persists the configuration.
   /// \param x signed step used for text-speed changes and as toggle trigger for boolean settings.
   void set(int32_t x);

   /// \brief draws the game screen layers and label text.
   /// \param window render target receiving the game screen.
   /// \param states render states forwarded to draw calls.
   void draw(sf::RenderTarget& window, sf::RenderStates states) override;

   Selection _selection = Selection::TextSpeed;

private:
   sf::FloatRect _row_help_base_rect;   //!< help text reference rect for row 0 (Text Speed)
   sf::FloatRect _row_value_base_rect;  //!< value text reference rect for row 0 (Text Speed)

   std::unique_ptr<sf::Text> _textspeed_label;
   std::unique_ptr<sf::Text> _textspeed_help_text;
   std::unique_ptr<sf::Text> _textspeed_value_text;
   std::unique_ptr<sf::Text> _rumble_label;
   std::unique_ptr<sf::Text> _rumble_help_text;
   std::unique_ptr<sf::Text> _rumble_value_text;
   std::unique_ptr<sf::Text> _autopause_label;
   std::unique_ptr<sf::Text> _autopause_help_text;
   std::unique_ptr<sf::Text> _autopause_value_text;
};
