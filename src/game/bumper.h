#ifndef BUMPER_H
#define BUMPER_H

#include <SFML/Graphics.hpp>

class Bumper
{

public:

   enum class Type {
     Vertical,
     HorizontalLeft,
     HorizontalRight
   };

   enum class State {
      Inactive,
      Expanding,
      Contractings
   };


private:

   // Type mType = Type::Vertical;
   // State mState = State::Inactive;
   // float mCycleTimeInS = 0.5f;
   sf::Time mStartTime;


public:

   Bumper();
   void touch();
   void update(const sf::Time& dt);
};

#endif // BUMPER_H
