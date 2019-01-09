#pragma once

class GameControllerBallVector
{
   public:

      GameControllerBallVector(float x, float y)
         : mX(x),
           mY(y)
      {
      }

      GameControllerBallVector()
      {
      }

      float* getXPtr()
      {
         return &mX;
      }

      float* getYPtr()
      {
         return &mY;
      }

   protected:

      float mX = 0.0f;
      float mY = 0.0f;
};

