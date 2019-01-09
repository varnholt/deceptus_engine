// header
#include "gamejoystickmapping.h"

#include "constants.h"

#include "../../SDL/include/SDL.h"


GameJoystickMapping::GameJoystickMapping()
 : mKeysPressed(0),
   mKeysPressedPrevious(0),
   mButtonBombId(0),
   mButtonKickId(0),
   mAxisThreshold(3200),
   mHatValue(SDL_HAT_CENTERED),
   mAxisXValue(0),
   mAxisYValue(0),
   mAxisKeys(0),
   mHatKeys(0)
{
}


void GameJoystickMapping::write(const GameControllerInfo& info)
{
   mJoystickInfo = info;

   bool hatsChanged = false;
   bool axesChanged = false;

   hatsChanged = processHats();
   axesChanged = processAxes();

   // use hat values
   if (hatsChanged)
   {
      setKeysPressed(getHatKeys());
   }

   // use axis values
   if (axesChanged)
   {
      setKeysPressed(getAxisKeys());

      int x = 0;
      int y = 0;

      getAxisValues(x, y);
   }

   // merge keys pressed with buttons
   processButtons();

   updateKeysPressed();
   setKeysPressedPrevious(getKeysPressed());
}


void GameJoystickMapping::setHatValue(int hat)
{
   mHatValue = hat;
}


int GameJoystickMapping::getHatValue() const
{
   return mHatValue;
}


void GameJoystickMapping::setAxisValues(int x, int y)
{
   mAxisXValue = x;
   mAxisYValue = y;
}


void GameJoystickMapping::getAxisValues(int &x, int &y)
{
   x = mAxisXValue;
   y = mAxisYValue;
}


void GameJoystickMapping::setKeysPressed(int keys)
{
   mKeysPressed = keys;
}


int GameJoystickMapping::getKeysPressed() const
{
   return mKeysPressed;
}


void GameJoystickMapping::setKeysPressedPrevious(int keys)
{
   mKeysPressedPrevious = keys;
}


int GameJoystickMapping::getKeysPressedPrevious() const
{
   return mKeysPressedPrevious;
}


void GameJoystickMapping::setAxisKeys(int keys)
{
   mAxisKeys = keys;
}


int GameJoystickMapping::getAxisKeys() const
{
   return mAxisKeys;
}


void GameJoystickMapping::setHatKeys(int keys)
{
   mHatKeys = keys;
}


int GameJoystickMapping::getHatKeys() const
{
   return mHatKeys;
}


void GameJoystickMapping::setAxisThreshold(int threshold)
{
   mAxisThreshold = threshold;
}


int GameJoystickMapping::getAxisThreshold()
{
   return mAxisThreshold;
}


void GameJoystickMapping::updateKeysPressed()
{
   // up
   if (
             (getKeysPressed()         & KeyPressedUp)
         &&! (getKeysPressedPrevious() & KeyPressedUp)
   )
   {
      printf("KeyPressedUp pressed\n");
      // emit keyPressed(controllerSettings->getUpKey());
   }

   if (
             (getKeysPressedPrevious() & KeyPressedUp)
         &&! (getKeysPressed()         & KeyPressedUp)
   )
   {
      printf("KeyPressedUp released\n");
      // emit keyReleased(controllerSettings->getUpKey());
   }

   // down
   if (
             (getKeysPressed()         & KeyPressedDown)
         &&! (getKeysPressedPrevious() & KeyPressedDown)
   )
   {
      printf("KeyPressedDown pressed\n");
      // emit keyPressed(controllerSettings->getDownKey());
   }

   if (
             (getKeysPressedPrevious() & KeyPressedDown)
         &&! (getKeysPressed()         & KeyPressedDown)
   )
   {
      printf("KeyPressedDown released\n");
      // emit keyReleased(controllerSettings->getDownKey());
   }

   // left
   if (
             (getKeysPressed()         & KeyPressedLeft)
         &&! (getKeysPressedPrevious() & KeyPressedLeft)
   )
   {
      printf("KeyPressedLeft pressed\n");
      // emit keyPressed(controllerSettings->getLeftKey());
   }

   if (
             (getKeysPressedPrevious() & KeyPressedLeft)
         &&! (getKeysPressed()         & KeyPressedLeft)
   )
   {
      printf("KeyPressedLeft released\n");
      // emit keyReleased(controllerSettings->getLeftKey());
   }

   // right
   if (
             (getKeysPressed()         & KeyPressedRight)
         &&! (getKeysPressedPrevious() & KeyPressedRight)
   )
   {
      printf("KeyPressedRight pressed\n");
      // emit keyPressed(controllerSettings->getRightKey());
   }

   if (
             (getKeysPressedPrevious() & KeyPressedRight)
         &&! (getKeysPressed()         & KeyPressedRight)
   )
   {
      printf("KeyPressedRight released\n");
      // emit keyReleased(controllerSettings->getRightKey());
   }

   // bomb
   /*
   if (
             (getKeysPressed()         & Constants::KeyBomb)
         &&! (getKeysPressedPrevious() & Constants::KeyBomb)
   )
   {
      emit keyPressed(controllerSettings->getBombKey());
   }

   if (
             (getKeysPressedPrevious() & Constants::KeyBomb)
         &&! (getKeysPressed()         & Constants::KeyBomb)
   )
   {
      emit keyReleased(controllerSettings->getBombKey());
   }
   */
}


bool GameJoystickMapping::processHats()
{
   bool changed = false;

   // use hat values
   if (mJoystickInfo.getHatValues().size() > 0)
   {
      setHatValue(mJoystickInfo.getHatValues()[0]);

      int keysModified = getHatKeys();
      int keysPrevious = getHatKeys();

      switch (getHatValue())
      {
         case SDL_HAT_CENTERED:
            keysModified &= ~KeyPressedUp;
            keysModified &= ~KeyPressedDown;
            keysModified &= ~KeyPressedLeft;
            keysModified &= ~KeyPressedRight;
            break;
         case SDL_HAT_UP:
            keysModified |= KeyPressedUp;
            keysModified &= ~KeyPressedDown;
            keysModified &= ~KeyPressedLeft;
            keysModified &= ~KeyPressedRight;
            break;
         case SDL_HAT_RIGHT:
            keysModified |= KeyPressedRight;
            keysModified &= ~KeyPressedUp;
            keysModified &= ~KeyPressedDown;
            keysModified &= ~KeyPressedLeft;
            break;
         case SDL_HAT_DOWN:
            keysModified |= KeyPressedDown;
            keysModified &= ~KeyPressedUp;
            keysModified &= ~KeyPressedLeft;
            keysModified &= ~KeyPressedRight;
            break;
         case SDL_HAT_LEFT:
            keysModified |= KeyPressedLeft;
            keysModified &= ~KeyPressedUp;
            keysModified &= ~KeyPressedDown;
            keysModified &= ~KeyPressedRight;
            break;
         case SDL_HAT_RIGHTUP:
            keysModified |= KeyPressedRight;
            keysModified |= KeyPressedUp;
            keysModified &= ~KeyPressedDown;
            keysModified &= ~KeyPressedLeft;
            break;
         case SDL_HAT_RIGHTDOWN:
            keysModified |= KeyPressedRight;
            keysModified |= KeyPressedDown;
            keysModified &= ~KeyPressedUp;
            keysModified &= ~KeyPressedLeft;
            break;
         case SDL_HAT_LEFTUP:
            keysModified |= KeyPressedLeft;
            keysModified |= KeyPressedUp;
            keysModified &= ~KeyPressedDown;
            keysModified &= ~KeyPressedRight;
            break;
         case SDL_HAT_LEFTDOWN:
            keysModified |= KeyPressedLeft;
            keysModified |= KeyPressedDown;
            keysModified &= ~KeyPressedUp;
            keysModified &= ~KeyPressedRight;
            break;
      }

      changed = (keysPrevious != keysModified);
      setHatKeys(keysModified);
   }

   return changed;
}


bool GameJoystickMapping::processButtons()
{
   bool changed = false;
   int buttons = 0;

   if (mJoystickInfo.getButtonValues().size() > 0)
   {
      int keysPressed = getKeysPressed();

      // there's only one thing to do by triggering a button
      // => drop a bomb
      bool bombdropped = false;

      int buttonId = 0;
      for (bool val : mJoystickInfo.getButtonValues())
      {
         if (val)
         {
            buttons |= 1 << buttonId;

            bombdropped = true;

            // this is not perfect for the controller options part,
            // but the number of shits given is 0
            break;
         }

         buttonId++;
      }

//      if (bombdropped)
//      {
//         setKeysPressed(getKeysPressed() | Constants::KeyBomb);
//      }
//      else
//      {
//         setKeysPressed(getKeysPressed() & ~Constants::KeyBomb);
//      }

      changed = (keysPressed != getKeysPressed());
   }

//   if (changed)
//   {
//      emit button(buttons);
//   }

   return changed;
}


bool GameJoystickMapping::processAxes()
{
   bool changed = false;

//   GameSettings::ControllerSettings* controllerSettings =
//      GameSettings::getInstance()->getControllerSettings();

   int axis1 = 0; // controllerSettings->getAnalogueAxis1();
   int axis2 = 1; // controllerSettings->getAnalogueAxis2();

   const std::vector<int>& axisValues = mJoystickInfo.getAxisValues();

   if (axisValues.size() >= (axis2 + 1))
   {
      int keysModified = getAxisKeys();
      int keysPrevious = getAxisKeys();

      int x = 0;
      int y = 0;

      setAxisValues(
         axisValues.at(axis1),
         axisValues.at(axis2)
      );

      getAxisValues(x, y);

      // x axis
      if (x < -mAxisThreshold)
      {
         // left
         keysModified |= KeyPressedLeft;
         keysModified &= ~KeyPressedRight;
      }
      else if (x > mAxisThreshold)
      {
         // right
         keysModified |= KeyPressedRight;
         keysModified &= ~KeyPressedLeft;
      }
      else
      {
         // left and right released
         keysModified &= ~KeyPressedLeft;
         keysModified &= ~KeyPressedRight;
      }

      // y axis
      if (y < -mAxisThreshold)
      {
         // up
         keysModified |= KeyPressedUp;
         keysModified &= ~KeyPressedDown;
      }
      else if (y > mAxisThreshold)
      {
         // down
         keysModified |= KeyPressedDown;
         keysModified &= ~KeyPressedUp;
      }
      else
      {
         // up and down released
         keysModified &= ~KeyPressedUp;
         keysModified &= ~KeyPressedDown;
      }

      changed = (keysPrevious != keysModified);
      setAxisKeys(keysModified);
   }

   return changed;
}

