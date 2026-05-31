#pragma once

#include "menuscreen.h"

#include <array>

/// \brief name-entry screen that edits the player name before starting a new save slot.
class MenuScreenNameSelect : public MenuScreen
{
public:
   /// \brief initializes name-entry assets, character table, and PSD layout.
   MenuScreenNameSelect();

   /// \brief processes text entry, cursor movement, delete, confirm, and cancel keys.
   /// \param key key that was pressed.
   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   /// \brief tracks shift key release state for keyboard character casing.
   /// \param key key that was released.
   void keyboardKeyReleased(sf::Keyboard::Key key) override;

   /// \brief maps controller X to deleting the last character.
   void controllerButtonX() override;

   /// \brief maps controller Y to inserting the character under the on-screen cursor.
   void controllerButtonY() override;

   /// \brief initializes cursor anchors, name bounds, and initial suggested username.
   void loadingFinished() override;

   /// \brief updates cursor position and prompt layers.
   void updateLayers();

   /// \brief draws PSD layers and the dynamic entered-name text.
   /// \param window render target receiving the screen output.
   /// \param states render states forwarded to draw calls.
   void draw(sf::RenderTarget& window, sf::RenderStates states) override;

   /// \brief moves the character-grid cursor up by one row.
   void up();

   /// \brief moves the character-grid cursor down by one row.
   void down();

   /// \brief moves the character-grid cursor left by one column.
   void left();

   /// \brief moves the character-grid cursor right by one column.
   void right();

   /// \brief confirms the entered name and resumes gameplay if the name is not empty.
   void select();

   /// \brief returns to the file select screen.
   void back();

private:
   /// \brief removes the last character from the entered name and refreshes text and layers.
   void chop();

   /// \brief appends a character to the entered name and refreshes text and layers.
   /// \param entered_char character to append.
   void appendChar(char entered_char);

   /// \brief centers the rendered name text inside the name field.
   void updateText();

   /// \brief seeds the entered name from environment username heuristics.
   void retrieveUsername();

   std::string _name;
   std::array<char, 13 * 5> _chars;

   sf::Vector2f _char_origin;
   sf::Vector2i _char_offset;

   sf::Rect<float> _name_rect;

   sf::Font _font;
   std::unique_ptr<sf::Text> _text;

   std::unique_ptr<sf::Text> _text_cancel_button;
   std::unique_ptr<sf::Text> _text_delete_button;
   std::unique_ptr<sf::Text> _text_confirm_button;

   int32_t _shift = 0;
};
