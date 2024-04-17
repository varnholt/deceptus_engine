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
#include "sdl/include/SDL.h"

class GameController
{
public:
   using ControllerCallback = std::function<void()>;

   struct ThresholdCallback
   {
      enum class Boundary
      {
         Upper,
         Lower
      };

      SDL_GameControllerAxis _axis = SDL_CONTROLLER_AXIS_INVALID;
      Boundary _boundary = Boundary::Upper;
      float _threshold = 0.3f;
      float _value = 0.0f;
      bool _initialized = false;
      ControllerCallback _callback;
   };

   //! constructor
   GameController() = default;

   //! destructor
   virtual ~GameController();

   // joystick management

   //! get number of joysticks
   virtual int32_t getJoystickCount() const;

   //! active joystick
   virtual void activate(int32_t id);

   //! getter for active joystick
   virtual int32_t getActiveJoystickId();

   // information about the current joystick

   //! check if given joystick id is valid
   bool validId(int32_t id) const;

   //! getter for the joystick's name
   virtual std::string getName(int32_t id) const;

   //! getter for the axis count
   virtual int32_t getAxisCount();

   //! getter for the ball count
   virtual int32_t getBallCount();

   //! getter for the hat count
   virtual int32_t getHatCount();

   //! update axis and button infos
   virtual void update();

   //! rumble test
   virtual void rumbleTest();

   //! start rumble effect
   virtual void rumble(float intensity, int32_t rumble_duration_ms);

   //! get button type by button id
   SDL_GameControllerButton getButtonType(int32_t buttonId) const;

   //! get button id by button type
   int32_t getButtonId(SDL_GameControllerButton) const;

   //! getter for axis id by axis type
   int32_t getAxisIndex(SDL_GameControllerAxis) const;

   //! getter for joystick info object
   const GameControllerInfo& getInfo() const;

   void addButtonPressedCallback(SDL_GameControllerButton, const ControllerCallback&);
   void removeButtonPressedCallback(SDL_GameControllerButton, const ControllerCallback&);
   void addButtonReleasedCallback(SDL_GameControllerButton, const ControllerCallback&);
   void addAxisThresholdExceedCallback(const ThresholdCallback& threshold);

protected:
   //! bind the dpad buttons
   void bindDpadButtons();

   //! check if button is dpad button
   bool isDpadButton(int32_t button) const;

private:
   void callPressedCallbacks(const SDL_GameControllerButton button);
   void callReleasedCallbacks(const SDL_GameControllerButton button);

   GameControllerInfo _info;

   SDL_Joystick* _joystick = nullptr;
   SDL_GameController* _controller = nullptr;

   SDL_GameControllerButtonBind _dpad_bind_up;
   SDL_GameControllerButtonBind _dpad_bind_down;
   SDL_GameControllerButtonBind _dpad_bind_left;
   SDL_GameControllerButtonBind _dpad_bind_right;

   std::map<SDL_GameControllerAxis, std::vector<ThresholdCallback>> _threshold_callbacks;
   std::map<SDL_GameControllerButton, std::vector<ControllerCallback>> _button_pressed_callbacks;
   std::map<SDL_GameControllerButton, std::vector<ControllerCallback>> _button_released_callbacks;
};
