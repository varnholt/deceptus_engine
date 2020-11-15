#ifndef GAMEJOYSTICKMAPPING_H
#define GAMEJOYSTICKMAPPING_H


// joystick
#include "../framework/joystick/gamecontrollerinfo.h"

class GameControllerMapping
{
   public:

      //! constructor
      GameControllerMapping();

      void updateKeysPressed();

      //! setter for hat value
      void setHatValue(int hat);

      //! getter for hat value
      int getHatValue() const;

      //! setter for axis values
      void setAxisValues(int x, int y);

      //! getter for axis value
      void getAxisValues(int& x, int&y);

      //! setter for keys pressed
      void setKeysPressed(int keys);

      //! getter for keys pressed
      int getKeysPressed() const;

      //! setter for keys pressed previous
      void setKeysPressedPrevious(int keys);

      //! getter for keys pressed previous
      int getKeysPressedPrevious() const;


      void setAxisKeys(int keys);
      int getAxisKeys() const;

      void setHatKeys(int keys);
      int getHatKeys() const;

      void setAxisThreshold(int threshold);
      int getAxisThreshold();


      //! incoming data
      void write(const GameControllerInfo&);


   protected:

      //! process axes
      bool processAxes();

      //! process hats
      bool processHats();


      //! joystick info coming from the joystick handler
      GameControllerInfo mJoystickInfo;

      //! keys pressed in this call
      int mKeysPressed;

      //! keys pressed in prevous call
      int mKeysPressedPrevious;

      // button ids

      //! key mapping for bomb
      int mButtonBombId;

      //! key mapping for kick
      int mButtonKickId;


      // thresholds

      //! axis threshold
      int mAxisThreshold;


      int mHatValue;

      int mAxisXValue;
      int mAxisYValue;

      int mAxisKeys;
      int mHatKeys;

};

#endif // GAMEJOYSTICKMAPPING_H

