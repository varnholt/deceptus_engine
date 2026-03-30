#pragma once

#include <math.h>  // added here since sdl wants to define its own M_PI
#include <functional>
#include <map>
#include <string>
#include "gamecontrollerinfo.h"

// SDL
#ifdef TARGET_OS_MAC
#ifdef MAC_OS_X_VERSION_MIN_REQUIRED
#undef MAC_OS_X_VERSION_MIN_REQUIRED
#endif
#define MAC_OS_X_VERSION_MIN_REQUIRED MAC_OS_X_VERSION_10_9
#endif
#include <SDL3/SDL.h>

///
/// \brief Wraps SDL gamepad/joystick input, state tracking, and callbacks.
///
class GameController
{
public:
   using ControllerCallback = std::function<void()>;

   ///
   /// \brief Configures a callback that fires when an axis crosses a threshold.
   ///
   struct ThresholdCallback
   {
      enum class Boundary
      {
         Upper,
         Lower
      };

      SDL_GamepadAxis _axis = SDL_GAMEPAD_AXIS_INVALID;
      Boundary _boundary = Boundary::Upper;
      float _threshold = 0.3f;
      float _value = 0.0f;
      bool _initialized = false;
      ControllerCallback _callback;
   };

   GameController() = default;

   ///
   /// \brief Closes the active SDL gamepad handle.
   ///
   virtual ~GameController();

   ///
   /// \brief Returns the number of connected joysticks.
   /// \return Connected joystick count.
   ///
   virtual int32_t getJoystickCount() const;

   ///
   /// \brief Opens the joystick or gamepad with the given SDL device id.
   /// \param id SDL joystick device id.
   ///
   virtual void activate(int32_t id);

   ///
   /// \brief Returns the SDL id of the currently active joystick.
   /// \return Active joystick id.
   ///
   virtual int32_t getActiveJoystickId();

   ///
   /// \brief Returns the display name of the joystick identified by id.
   /// \param id SDL joystick device id.
   /// \return Joystick name.
   ///
   virtual std::string getName(int32_t id) const;

   ///
   /// \brief Returns the number of axes exposed by the active joystick.
   /// \return Axis count.
   ///
   virtual int32_t getAxisCount();

   ///
   /// \brief Returns the number of trackballs exposed by the active joystick.
   /// \return Ball count.
   ///
   virtual int32_t getBallCount();

   ///
   /// \brief Returns the number of hats exposed by the active joystick.
   /// \return Hat count.
   ///
   virtual int32_t getHatCount();

   ///
   /// \brief Polls SDL input and updates cached axis, button, and hat states.
   ///
   /// this also dispatches registered press/release callbacks and axis-threshold callbacks.
   ///
   virtual void update();

   ///
   /// \brief Plays a full-strength rumble for 2 seconds.
   ///
   virtual void rumbleTest();

   ///
   /// \brief Starts gamepad rumble with the requested intensity and duration.
   /// \param intensity Rumble strength in [0.0, 1.0].
   /// \param rumble_duration_ms Duration in milliseconds.
   ///
   virtual void rumble(float intensity, int32_t rumble_duration_ms);

   ///
   /// \brief Resolves a physical button id to its mapped SDL gamepad button.
   /// \param button_id Physical button id from the current SDL binding.
   /// \return Mapped gamepad button, or `SDL_GAMEPAD_BUTTON_INVALID`.
   ///
   SDL_GamepadButton getButtonType(int32_t button_id) const;

   ///
   /// \brief Resolves the hardware axis index for a mapped SDL gamepad axis.
   /// \param axis SDL gamepad axis.
   /// \return Hardware axis index, or `-1` when not found.
   ///
   int32_t getAxisIndex(SDL_GamepadAxis axis) const;

   ///
   /// \brief Returns the most recently sampled controller state snapshot.
   /// \return Cached controller info.
   ///
   const GameControllerInfo& getInfo() const;

   ///
   /// \brief Registers a callback for button press transitions.
   /// \param button Button that should trigger the callback.
   /// \param callback Callback to run on press.
   ///
   void addButtonPressedCallback(SDL_GamepadButton button, const ControllerCallback& callback);

   ///
   /// \brief Removes a previously registered press callback for a button.
   /// \param button Button whose callback should be removed.
   /// \param callback Callback to remove.
   ///
   void removeButtonPressedCallback(SDL_GamepadButton button, const ControllerCallback& callback);

   ///
   /// \brief Registers a callback for button release transitions.
   /// \param button Button that should trigger the callback.
   /// \param callback Callback to run on release.
   ///
   void addButtonReleasedCallback(SDL_GamepadButton button, const ControllerCallback& callback);

   ///
   /// \brief Registers a callback for axis threshold crossings.
   /// \param threshold Threshold configuration and callback.
   ///
   void addAxisThresholdExceedCallback(const ThresholdCallback& threshold);

protected:
   ///
   /// \brief Caches dpad bindings from the active SDL gamepad mapping.
   ///
   void bindDpadButtons();

   ///
   /// \brief Returns whether a physical button id is mapped to the dpad.
   /// \param button Physical button id.
   /// \return `true` when the button is one of the dpad bindings.
   ///
   bool isDpadButton(int32_t button) const;

private:
   ///
   /// \brief Dispatches callbacks registered for a pressed button transition.
   /// \param button Button that became pressed.
   ///
   void callPressedCallbacks(const SDL_GamepadButton button);

   ///
   /// \brief Dispatches callbacks registered for a released button transition.
   /// \param button Button that became released.
   ///
   void callReleasedCallbacks(const SDL_GamepadButton button);

   GameControllerInfo _info;

   SDL_Joystick* _joystick = nullptr;
   SDL_Gamepad* _gamepad = nullptr;

   SDL_GamepadBinding _dpad_bind_up;
   SDL_GamepadBinding _dpad_bind_down;
   SDL_GamepadBinding _dpad_bind_left;
   SDL_GamepadBinding _dpad_bind_right;

   std::map<SDL_GamepadAxis, std::vector<ThresholdCallback>> _threshold_callbacks;
   std::map<SDL_GamepadButton, std::vector<ControllerCallback>> _button_pressed_callbacks;
   std::map<SDL_GamepadButton, std::vector<ControllerCallback>> _button_released_callbacks;
};
