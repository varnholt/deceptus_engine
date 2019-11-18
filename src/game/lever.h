#pragma once

#include <functional>
#include <stdint.h>

class Lever
{

public:

   using Callback = std::function<void(bool)>;


   enum class State {
      Off,
      On,
   };

   Lever() = default;

   void toggle();
   void setCallback(const Callback& callback);

private:

   State mState = State::Off;
   Callback mCallback;
};


