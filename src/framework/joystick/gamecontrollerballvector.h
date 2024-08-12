#pragma once

class GameControllerBallVector
{
public:
   GameControllerBallVector(float x, float y) : _x(x), _y(y)
   {
   }

   GameControllerBallVector()
   {
   }

   float* getXPtr()
   {
      return &_x;
   }

   float* getYPtr()
   {
      return &_y;
   }

protected:
   float _x = 0.0f;
   float _y = 0.0f;
};
