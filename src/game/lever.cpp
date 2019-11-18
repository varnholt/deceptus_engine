#include "lever.h"


void Lever::toggle()
{
   switch (mState)
   {
      case State::Off:
         mState = State::On;
         break;
      case State::On:
         mState = State::Off;
         break;
   }

   mCallback(mState == State::Off);
}


void Lever::setCallback(const Callback& callback)
{
   mCallback = callback;
}
