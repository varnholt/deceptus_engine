#pragma once

#include "menuscreen.h"

#include "game/constants.h"

#include <SFML/Graphics.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

/// \brief controls screen with device selection and per-device input binding assignment.
///
/// lists keyboard as the first entry followed by all currently connected SDL controllers.
/// selecting an entry loads the corresponding input profile and shows an inline assignment
/// table where each action can be rebound. pressing back saves the bindings and returns
/// to the device list.
class MenuScreenControls : public MenuScreen
{
public:
   /// \brief identifies whether keyboard or controller bindings are being edited.
   enum class DeviceMode
   {
      Keyboard,    //!< editing keyboard key assignments; Enter captures a key
      Controller,  //!< editing controller button assignments; Enter or Y captures a button
   };

   /// \brief initializes font, text, and highlight shapes; loads the controls PSD.
   MenuScreenControls();

   /// \brief draws the active view on top of the PSD background layers.
   /// \param window render target receiving the screen output.
   /// \param states render states forwarded to draw calls.
   void draw(sf::RenderTarget& window, sf::RenderStates states) override;

   /// \brief updates prompt layer visibility after the PSD finishes loading.
   void loadingFinished() override;

   /// \brief resets to device selection and rebuilds the device list on each show.
   void showEvent() override;

   /// \brief polls controller button transitions when waiting for a button assignment.
   /// \param dt frame delta time.
   void update(const sf::Time& dt) override;

   /// \brief routes navigation and assignment keys to the active view.
   /// \param key key that was pressed.
   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   /// \brief starts controller button assignment for the selected action in the assignment view.
   void controllerButtonY() override;

   /// \brief updates button prompt layer visibility based on the current input device.
   void updateLayers();

private:
   /// \brief identifies the interaction state within the assignment view.
   enum class AssignmentState
   {
      Idle,              //!< browsing the action list
      WaitingForKey,     //!< next key press is captured as a new binding
      WaitingForButton,  //!< next non-dpad controller button press is captured
   };

   /// \brief one entry in the device list (keyboard or a specific controller).
   struct DeviceEntry
   {
      std::string display_name;  //!< shown in the list (e.g. "Keyboard", "Xbox Controller")
      std::string guid;          //!< empty for keyboard; SDL GUID hex string for controllers
      int32_t joystick_id = -1;  //!< -1 for keyboard; SDL joystick id for controllers
   };

   /// \brief rebuilds _device_entries from the current controller integration state.
   void rebuildDeviceList();

   /// \brief loads bindings for the device at the given index in the device list.
   /// \param index index into _device_entries.
   void loadDevice(int32_t index);

   /// \brief cycles the active device by direction (-1 or +1) and reloads its bindings.
   /// \param direction -1 for previous device, +1 for next device.
   void cycleDevice(int32_t direction);

   /// \brief moves cursor up one row in the active view.
   void up();

   /// \brief moves cursor down one row in the active view.
   void down();

   /// \brief confirms the current selection in the active view.
   void select();

   /// \brief goes back: saves bindings when in assignment view, returns to options from device selection.
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
   std::unique_ptr<sf::Text> _text;       //!< reused per draw call for all labels and table cells
   sf::RectangleShape _cursor_highlight;  //!< highlight rect drawn behind the selected row

   // device selection state
   std::vector<DeviceEntry> _device_entries;
   int32_t _device_row_index = 0;  //!< selected row in the device list

   // input assignment state
   AssignmentState _assignment_state = AssignmentState::Idle;
   int32_t _action_row_index = 0;                         //!< 0..num_actions-1 for action rows; num_actions for Reset Defaults
   KeyPressed _pending_action = KeyPressedUp;             //!< action being reassigned while not Idle
   std::vector<bool> _previous_controller_button_values;  //!< previous-frame button states for edge detection
   DeviceMode _device_mode = DeviceMode::Keyboard;        //!< current device being configured
   std::string _device_name{"Keyboard"};                  //!< display name shown in the assignment view title
};
