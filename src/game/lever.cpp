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
         Callback(10);
         break;
   }
}
