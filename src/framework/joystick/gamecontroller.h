#pragma once

#include "gamecontrollerinfo.h"
#include <functional>
#include <map>
#include <math.h> // added here since sdl wants to define its own M_PI
#include <string>


// SDL
#ifdef TARGET_OS_MAC
#ifdef MAC_OS_X_VERSION_MIN_REQUIRED
#undef MAC_OS_X_VERSION_MIN_REQUIRED
#endif
#define MAC_OS_X_VERSION_MIN_REQUIRED MAC_OS_X_VERSION_10_9
#endif
#include "../../SDL/include/SDL.h"


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
      GameController();

      //! destructor
      virtual ~GameController();

      // joystick management

      //! get number of joysticks
      virtual int getJoystickCount() const;

      //! setter for active joystick
      virtual void setActiveJoystick(int id);

      //! getter for active joystick
      virtual int getActiveJoystick();


      // information about the current joystick

      //! getter for the joystick's name
      virtual std::string getName(int id) const;

      //! getter for the axis count
      virtual int getAxisCount(int id);

      //! getter for the ball count
      virtual int getBallCount(int id);

      //! getter for the hat count
      virtual int getHatCount(int id);

      //! update axis and button infos
      virtual void update();


      //! rumble test
      virtual void rumbleTest();

      //! start rumble effect
      virtual void rumble(float intensity, int ms);


      //! get button type by button id
      SDL_GameControllerButton getButtonType(int buttonId) const;

      //! get button id by button type
      int32_t getButtonId(SDL_GameControllerButton) const;

      //! getter for axis id by axis type
      int32_t getAxisIndex(SDL_GameControllerAxis) const;

      //! getter for joystick info object
      const GameControllerInfo& getInfo() const;

      void addButtonPressedCallback(
         SDL_GameControllerButton,
         const ControllerCallback&
      );

      void removeButtonPressedCallback(
         SDL_GameControllerButton,
         const ControllerCallback&
      );

      void addButtonReleasedCallback(
         SDL_GameControllerButton,
         const ControllerCallback&
      );

      void addAxisThresholdExceedCallback(const ThresholdCallback& threshold);


   protected:

      //! clean up rumble effect
      void cleanupRumble();

      //! bind the dpad buttons
      void bindDpadButtons();

      //! check if button is dpad button
      bool isDpadButton(int button) const;


   private:

      GameControllerInfo _info;

      //! active joystick
      SDL_Joystick* _active_joystick = nullptr;

      SDL_GameController* _controller = nullptr;
      SDL_GameControllerButtonBind _dpad_bind_up;
      SDL_GameControllerButtonBind _dpad_bind_down;
      SDL_GameControllerButtonBind _dpad_bind_left;
      SDL_GameControllerButtonBind _dpad_bind_right;

      //! running haptic effect
      SDL_Haptic* _haptic = nullptr;

      std::map<SDL_GameControllerAxis, std::vector<ThresholdCallback>> _threshold_callbacks;
      std::map<SDL_GameControllerButton, std::vector<ControllerCallback>> _button_pressed_callbacks;
      std::map<SDL_GameControllerButton, std::vector<ControllerCallback>> _button_released_callbacks;
};

