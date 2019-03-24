#pragma once

#include "gamecontrollerinfo.h"
#include <functional>
#include <map>
#include <math.h> // added here since sdl wants to define its own M_PI
#include <string>


// SDL
#ifdef Q_OS_MAC
#ifdef MAC_OS_X_VERSION_MIN_REQUIRED
#undef MAC_OS_X_VERSION_MIN_REQUIRED
#endif
#define MAC_OS_X_VERSION_MIN_REQUIRED MAC_OS_X_VERSION_10_9
#endif
#include "../../SDL/include/SDL.h"


class GameController
{
   public:

      struct ThresholdCallback
      {
         enum class Boundary
         {
            Upper,
            Lower
         };

         SDL_GameControllerAxis mAxis = SDL_CONTROLLER_AXIS_INVALID;
         Boundary mBoundary = Boundary::Upper;
         float mThreshold = 0.3f;
         float mValue = -1.0f;
         std::function<void()> mCallback;
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
         std::function<void()>
      );

      void addButtonReleasedCallback(
         SDL_GameControllerButton,
         std::function<void()>
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

      GameControllerInfo mInfo;

      //! active joystick
      SDL_Joystick* mActiveJoystick = nullptr;

      SDL_GameController* mController = nullptr;
      SDL_GameControllerButtonBind mDpadUpBind;
      SDL_GameControllerButtonBind mDpadDownBind;
      SDL_GameControllerButtonBind mDpadLeftBind;
      SDL_GameControllerButtonBind mDpadRightBind;

      //! running haptic effect
      SDL_Haptic* mHaptic = nullptr;

      std::map<SDL_GameControllerAxis, ThresholdCallback> mThresholdCallbacks;
      std::map<SDL_GameControllerButton, std::vector<std::function<void()>>> mButtonPressedCallbacks;
      std::map<SDL_GameControllerButton, std::vector<std::function<void()>>> mButtonReleasedCallbacks;
};

