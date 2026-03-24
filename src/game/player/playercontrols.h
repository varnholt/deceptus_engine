#pragma once

#include "framework/joystick/gamecontrollerinfo.h"
#include "game/constants.h"
#include "game/io/eventserializer.h"
#include "game/player/playerinput.h"

#include <chrono>
#include <functional>

#include <SFML/Graphics.hpp>

/// \brief collects keyboard and controller input and exposes normalized player control queries.
class PlayerControls
{
public:
   /// \brief creates input state, event replay hooks, and playback-state integration.
   PlayerControls();

   /// \brief advances control state by updating key locks, replay, and previous movement flags.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt);

   using KeypressedCallback = std::function<void(sf::Keyboard::Key)>;

   /// \brief registers a callback invoked whenever a key press event is processed.
   /// \param callback callback that receives the pressed sfml key code.
   void addKeypressedCallback(const KeypressedCallback& callback);

   /// \brief determines whether a key flag is active after applying temporary lock overrides.
   /// \param flag key bit mask to query.
   /// \return true when the requested key state is currently active.
   bool hasFlag(KeyPressed flag) const;

   /// \brief handles a keyboard key press and refreshes internal key-bit state.
   /// \param key lookup key.
   void keyboardKeyPressed(sf::Keyboard::Key key);

   /// \brief handles a keyboard key release and clears internal key-bit state.
   /// \param key lookup key.
   void keyboardKeyReleased(sf::Keyboard::Key key);

   /// \brief synchronizes key-bit state with current realtime keyboard state.
   void forceSync();

   /// \brief gets the raw bit mask of pressed keyboard actions.
   /// \return current key bit field.
   int getKeysPressed() const;

   /// \brief overwrites the raw pressed-key bit field.
   /// \param keys new key bit field.
   void setKeysPressed(int32_t keys);

   /// \brief determines whether camera-panorama control input is currently active.
   /// \return true when look key or right-stick movement exceeds configured thresholds.
   bool isCpanControlActive() const;

   /// \brief queries the current state of a controller button.
   /// \param button_enum SDL gamepad button enum value.
   /// \return true when that controller button is currently pressed.
   bool isControllerButtonPressed(int32_t button_enum) const;

   /// \brief determines whether jump input is active from keyboard or controller.
   /// \return true when jump is pressed.
   bool isButtonAPressed() const;

   /// \brief determines whether action input is active from keyboard or controller.
   /// \return true when action is pressed.
   bool isButtonBPressed() const;

   /// \brief determines whether slot-1 input is active from keyboard or controller.
   /// \return true when slot 1 is pressed.
   bool isButtonXPressed() const;  // slot 1
   /// \brief determines whether slot-2 input is active from keyboard or controller.
   /// \return true when slot 2 is pressed.
   bool isButtonYPressed() const;  // slot 2
   /// \brief determines whether upward digital input is active.
   /// \return true when up is pressed.
   bool isUpButtonPressed() const;

   /// \brief determines whether downward digital input is active.
   /// \return true when down is pressed.
   bool isDownButtonPressed() const;

   /// \brief determines whether drop-through input is active.
   /// \return true when jump is held while moving down.
   bool isDroppingDown() const;

   /// \brief determines whether horizontal movement input currently points right.
   /// \return true when right movement is active.
   bool isMovingRight() const;

   /// \brief determines whether horizontal movement input currently points left.
   /// \return true when left movement is active.
   bool isMovingLeft() const;

   /// \brief determines whether movement input currently points down.
   /// \param analog_threshold minimum absolute stick value needed for analog down input.
   /// \return true when down movement is active.
   bool isMovingDown(float analog_threshold = 0.3f) const;

   /// \brief determines whether movement input currently points up.
   /// \param analog_threshold minimum absolute stick value needed for analog up input.
   /// \return true when up movement is active.
   bool isMovingUp(float analog_threshold = 0.3f) const;

   /// \brief determines whether any horizontal movement input is active.
   /// \return true when either left or right movement is active.
   bool isMovingHorizontally() const;

   /// \brief gets cached joystick state collected by the controller integration.
   /// \return current controller info snapshot.
   const GameControllerInfo& getJoystickInfo() const;

   /// \brief stores the latest joystick state snapshot.
   /// \param joystickInfo controller state to cache for later queries.
   void setJoystickInfo(const GameControllerInfo& joystickInfo);

   /// \brief reports whether horizontal movement was active during the previous update.
   /// \return true when the previous frame had horizontal movement.
   bool wasMoving() const;

   /// \brief stores previous-frame horizontal movement state.
   /// \param was_moving true when horizontal movement was active.
   void setWasMoving(bool was_moving);

   /// \brief reports whether left movement was active during the previous update.
   /// \return true when previous-frame left movement was active.
   bool wasMovingLeft() const;

   /// \brief stores previous-frame left movement state.
   /// \param was_moving_left true when left movement was active.
   void setWasMovingLeft(bool was_moving_left);

   /// \brief reports whether right movement was active during the previous update.
   /// \return true when previous-frame right movement was active.
   bool wasMovingRight() const;

   /// \brief stores previous-frame right movement state.
   /// \param was_moving_right true when right movement was active.
   void setWasMovingRight(bool was_moving_right);

   /// \brief determines whether movement changed from moving to idle since the previous update.
   /// \return true when the player stopped horizontal movement this frame.
   bool changedToIdle() const;

   /// \brief determines whether movement changed from idle to moving since the previous update.
   /// \return true when the player started horizontal movement this frame.
   bool changedToMoving() const;

   enum class LockedState
   {
      Pressed,
      Released,
   };

   enum class Orientation
   {
      Undefined,
      Left,
      Right
   };

   /// \brief resolves the player's facing orientation from current input and orientation locks.
   /// \return requested orientation for this frame, or undefined when no orientation input exists.
   Orientation updateOrientation();

   /// \brief determines whether bend-down input is active while allowing camera-panorama state.
   /// \return true when down input exceeds the configured bend threshold.
   bool isBendDownActive() const;

   /// \brief reports whether the controller was the last input source.
   /// \return true when controller input was seen more recently than keyboard input.
   bool isControllerUsedLast() const;

   /// \brief locks facing orientation for a duration, optionally forcing a specific direction.
   /// \param duration lock duration.
   /// \param orientation orientation to force, or undefined to keep current orientation.
   void lockOrientation(std::chrono::milliseconds duration, Orientation = Orientation::Undefined);

   /// \brief locks one key state to pressed or released for a duration.
   /// \param key lookup key.
   /// \param state forced key state while locked.
   /// \param duration lock duration.
   void lockState(KeyPressed key, LockedState state, const std::chrono::milliseconds& duration);

   /// \brief applies the same temporary lock state to all supported player input keys.
   /// \param state forced key state while locked.
   /// \param duration lock duration.
   void lockAll(LockedState state, const std::chrono::milliseconds& duration);

   /// \brief reads horizontal controller input normalized to the range -1 to 1.
   /// \return normalized horizontal input, including dpad and lock-state overrides.
   float readControllerNormalizedHorizontal() const;

   /// \brief handles sfml input events and forwards them to keyboard state and replay recording.
   /// \param event sfml event to process.
   void handleEvent(const sf::Event& event);

private:
   /// \brief stores temporary forced state for one logical input key.
   struct LockedKey
   {
      std::chrono::milliseconds _locked_duration;
      LockedState _state{LockedState::Pressed};
      std::chrono::milliseconds _elapsed;

      /// \brief converts the locked state enum to its boolean pressed/released value.
      /// \return true when the lock enforces a pressed state.
      bool asBool() const;
   };

   /// \brief refreshes which input device was used last based on current controller activity.
   void updatePlayerInput();

   /// \brief advances lock timers and clears expired key locks.
   /// \param dt elapsed frame time.
   void updateLockedKeys(const sf::Time& dt);

   /// \brief looks up whether a key currently has an active lock override.
   /// \param key logical input key to query.
   /// \return iterator to the key lock entry or end when no lock exists.
   std::unordered_map<KeyPressed, LockedKey>::const_iterator readLockedState(KeyPressed) const;

   GameControllerInfo _joystick_info;

   int32_t _keys_pressed = 0;

   bool _was_moving = false;
   bool _was_moving_left = false;
   bool _was_moving_right = false;

   std::vector<KeypressedCallback> _keypressed_callbacks;
   PlayerInput _player_input;

   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

   HighResTimePoint _unlock_orientation_time_point;
   Orientation _locked_orientation = Orientation::Undefined;
   Orientation _last_requested_orientation = Orientation::Undefined;
   std::unordered_map<KeyPressed, LockedKey> _locked_keys;
   std::shared_ptr<EventSerializer> _event_serializer;
};
