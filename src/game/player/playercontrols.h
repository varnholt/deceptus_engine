#pragma once

#include "framework/joystick/gamecontrollerinfo.h"
#include "game/constants.h"
#include "game/player/playerinput.h"

#include <chrono>
#include <functional>

#include <SFML/Graphics.hpp>

class PlayerControls
{
public:
   PlayerControls() = default;

   void update(const sf::Time& dt);

   using KeypressedCallback = std::function<void(sf::Keyboard::Key)>;
   void addKeypressedCallback(const KeypressedCallback& callback);

   bool hasFlag(KeyPressed flag) const;

   void keyboardKeyPressed(sf::Keyboard::Key key);
   void keyboardKeyReleased(sf::Keyboard::Key key);
   void forceSync();

   int getKeysPressed() const;
   void setKeysPressed(int32_t keys);

   bool isLookingAround() const;
   bool isControllerButtonPressed(int32_t button_enum) const;
   bool isButtonXPressed() const;
   bool isButtonAPressed() const;
   bool isButtonBPressed() const;
   bool isUpButtonPressed() const;
   bool isDownButtonPressed() const;
   bool isDroppingDown() const;

   bool isMovingRight() const;
   bool isMovingLeft() const;
   bool isMovingDown(float analog_threshold = 0.3f) const;
   bool isMovingUp(float analog_threshold = 0.3f) const;
   bool isMovingHorizontally() const;

   const GameControllerInfo& getJoystickInfo() const;
   void setJoystickInfo(const GameControllerInfo& joystickInfo);

   bool wasMoving() const;
   void setWasMoving(bool was_moving);

   bool wasMovingLeft() const;
   void setWasMovingLeft(bool was_moving_left);

   bool wasMovingRight() const;
   void setWasMovingRight(bool was_moving_right);

   bool changedToIdle() const;
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

   Orientation updateOrientation();
   bool isBendDownActive() const;
   bool isControllerUsedLast() const;
   void lockOrientation(std::chrono::milliseconds duration, Orientation = Orientation::Undefined);
   void lockState(KeyPressed key, LockedState state, std::chrono::milliseconds duration);

   float readControllerNormalizedHorizontal() const;

private:
   struct LockedKey
   {
      std::chrono::milliseconds _locked_duration;
      LockedState _state{LockedState::Pressed};
      std::chrono::milliseconds _elapsed;
      bool asBool() const;
   };

   void updatePlayerInput();
   void updateLockedKeys(const sf::Time& dt);
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
};
