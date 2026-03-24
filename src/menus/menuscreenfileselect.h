#pragma once

#include <array>

#include "menuscreen.h"

/// \brief save-slot selection screen used for starting, loading, and deleting game files.
class MenuScreenFileSelect : public MenuScreen
{
public:
   /// \brief identifies the three save slots displayed on the file select screen.
   enum class Slot
   {
      A = 0,
      B = 1,
      C = 2
   };

   /// \brief initializes fonts, slot name labels, and PSD layout for file selection.
   MenuScreenFileSelect();

   /// \brief draws PSD layers and dynamic slot-name text overlays.
   /// \param window render target receiving the screen output.
   /// \param states render states forwarded to draw calls.
   void draw(sf::RenderTarget& window, sf::RenderStates states) override;

   /// \brief routes navigation, confirm, delete, and back keys to file-select actions.
   /// \param key key that was pressed.
   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   /// \brief maps controller X to save-slot deletion.
   void controllerButtonX() override;

   /// \brief loads save data from disk and updates slot layers.
   void loadingFinished() override;

   /// \brief refreshes slot presentation whenever the screen is shown.
   void showEvent() override;

private:
   /// \brief updates slot visuals, slot names, and button prompts from current save state data.
   void updateLayers();

   /// \brief moves selection one slot up without wrapping above the first slot.
   void up();

   /// \brief moves selection one slot down without wrapping below the last slot.
   void down();

   /// \brief enters name selection for empty slots or resumes gameplay from existing saves.
   void select();

   /// \brief returns to the main menu.
   void back();

   /// \brief asks for confirmation and deletes the selected save slot when confirmed.
   void remove();

   sf::Font _font;
   std::array<std::unique_ptr<sf::Text>, 3> _names;

   Slot _slot = Slot::A;
};
