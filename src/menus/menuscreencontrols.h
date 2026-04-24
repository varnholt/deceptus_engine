#pragma once

#include "menuscreen.h"

#include "game/constants.h"

#include <SFML/Graphics.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

/// \brief device-selector screen shown before entering the input assignment screen.
///
/// lists keyboard as the first entry followed by all currently connected SDL controllers.
/// selecting an entry loads the corresponding input profile and navigates to the assignment screen.
class MenuScreenControls : public MenuScreen
{
public:
   /// \brief initializes font, text, and background shapes for the device list.
   MenuScreenControls();

   /// \brief draws the device list with cursor highlight and navigation hints.
   /// \param window render target receiving the screen output.
   /// \param states render states forwarded to draw calls.
   void draw(sf::RenderTarget& window, sf::RenderStates states) override;

   /// \brief rebuilds the device list from currently connected controllers on each show.
   void showEvent() override;

   /// \brief routes navigation and confirm keys to device-selector actions.
   /// \param key key that was pressed.
   void keyboardKeyPressed(sf::Keyboard::Key key) override;

private:
   /// \brief one entry in the device list (keyboard or a specific controller).
   struct DeviceEntry
   {
      std::string display_name;  //!< shown in the list (e.g. "Keyboard", "Xbox Controller")
      std::string guid;          //!< empty for keyboard; SDL GUID hex string for controllers
      int32_t joystick_id = -1;  //!< -1 for keyboard; SDL joystick id for controllers
   };

   /// \brief rebuilds _device_entries from the current controller integration state.
   void rebuildDeviceList();

   /// \brief moves cursor up one row, clamped to first entry.
   void up();

   /// \brief moves cursor down one row, clamped to last entry.
   void down();

   /// \brief loads the selected device's profile and opens the assignment screen.
   void select();

   /// \brief returns to the options menu.
   void back();

   sf::Font _font;
   std::unique_ptr<sf::Text> _text;       //!< reused per draw call for all text
   sf::RectangleShape _background;        //!< full-screen dark panel
   sf::RectangleShape _cursor_highlight;  //!< highlight rect behind the selected row

   std::vector<DeviceEntry> _device_entries;
   int32_t _selected_row_index = 0;
};
