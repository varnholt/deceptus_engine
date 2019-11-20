#include "lever.h"


void Lever::toggle()
{
   if (mType == Type::TwoState)
   {
      switch (mState)
      {
         case State::Left:
            mState = State::Right;
            break;
         case State::Right:
            mState = State::Left;
            break;
         case State::Middle:
            break;
      }
   }

   else if (mType == Type::TriState)
   {
      switch (mState)
      {
         case State::Left:
         {
            mState = State::Middle;
            break;
         }
         case State::Middle:
         {
            if (mPreviousState == State::Left)
            {
               mState = State::Right;
            }
            else
            {
               mState = State::Left;
            }
            break;
         }
         case State::Right:
         {
            mState = State::Middle;
            break;
         }
      }

      mPreviousState = mState;
   }

   mCallback(static_cast<int32_t>(mState));
}


void Lever::setCallback(const Callback& callback)
{
   mCallback = callback;
}
