#pragma once

#include <functional>
#include <stdint.h>

class Lever
{

public:

   using Callback = std::function<void(int32_t)>;

   enum class Type {
      TwoState,
      TriState
   };

   enum class State {
      Left   = -1,
      Middle = 0,
      Right  = 1,
   };

   Lever() = default;

   void toggle();
   void setCallback(const Callback& callback);

private:

   Type mType = Type::TwoState;
   State mState = State::Left;
   State mPreviousState = State::Left;
   Callback mCallback;
};


