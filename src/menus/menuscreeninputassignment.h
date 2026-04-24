#pragma once

#include "menuscreen.h"

#include "game/constants.h"

#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <vector>

/// \brief input assignment screen for rebinding keyboard keys and controller buttons to game actions.
class MenuScreenInputAssignment : public MenuScreen
{
public:
   /// \brief identifies whether this screen is editing keyboard or controller bindings.
   enum class DeviceMode
   {
      Keyboard,    //!< editing keyboard key assignments; Enter captures a key
      Controller,  //!< editing controller button assignments; Enter/Y captures a button
   };

   /// \brief initializes font, text, and background shapes for the assignment list.
   MenuScreenInputAssignment();

   /// \brief draws the assignment list with current bindings, cursor highlight, and status text.
   /// \param window render target receiving the screen output.
   /// \param states render states forwarded to draw calls.
   void draw(sf::RenderTarget& window, sf::RenderStates states) override;

   /// \brief polls controller button transitions when waiting for a button assignment.
   /// \param dt frame delta time.
   void update(const sf::Time& dt) override;

   /// \brief routes navigation and assignment keys; captures the next key when waiting.
   /// \param key key that was pressed.
   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   /// \brief starts controller button assignment for the selected action when idle.
   void controllerButtonY() override;

   /// \brief re-reads current bindings from InputConfiguration on each show.
   void showEvent() override;

   /// \brief sets the device mode and display name before showing the screen.
   /// \param mode keyboard or controller editing mode.
   /// \param device_name human-readable name shown in the screen title.
   void setDeviceMode(DeviceMode mode, const std::string& device_name);

private:
   /// \brief identifies the current interaction state of the assignment screen.
   enum class AssignmentState
   {
      Idle,              //!< browsing the list
      WaitingForKey,     //!< next key press is captured as a new binding
      WaitingForButton,  //!< next non-dpad controller button press is captured as a new binding
   };

   /// \brief moves cursor up one row, clamped to the first row.
   void up();

   /// \brief moves cursor down one row, clamped to the Reset Defaults row.
   void down();

   /// \brief starts assignment for action rows or resets defaults for the Reset row.
   void select();

   /// \brief saves current bindings to disk and returns to the controls menu.
   void back();

   /// \brief copies default bindings for the current device mode and saves to disk.
   void resetDefaults();

   /// \brief assigns the given key to the pending action and returns to idle state.
   /// \param key keyboard key to bind.
   void completeKeyAssignment(sf::Keyboard::Key key);

   /// \brief assigns the given SDL button to the pending action and returns to idle state.
   /// \param sdl_button SDL gamepad button index to bind.
   void completeButtonAssignment(int32_t sdl_button);

   sf::Font _font;
   std::unique_ptr<sf::Text> _text;       //!< reused per draw call to render all table cells and labels
   sf::RectangleShape _background;        //!< full-screen dark panel
   sf::RectangleShape _cursor_highlight;  //!< highlight rect drawn behind the selected row
   AssignmentState _assignment_state = AssignmentState::Idle;
   int32_t _selected_row_index = 0;                       //!< 0..num_actions-1 for action rows; num_actions for Reset Defaults
   KeyPressed _pending_action = KeyPressedUp;             //!< action being reassigned while not Idle
   std::vector<bool> _previous_controller_button_values;  //!< previous-frame button states for edge detection
   DeviceMode _device_mode = DeviceMode::Keyboard;        //!< current device being configured
   std::string _device_name{"Keyboard"};                  //!< display name shown in the screen title
};
