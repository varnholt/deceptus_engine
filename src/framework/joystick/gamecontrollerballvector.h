#pragma once

///
/// \brief Stores one joystick trackball delta vector.
///
class GameControllerBallVector
{
public:
   ///
   /// \brief Constructs a ball vector with explicit components.
   /// \param x X component.
   /// \param y Y component.
   ///
   GameControllerBallVector(float x, float y) : _x(x), _y(y)
   {
   }

   ///
   /// \brief Constructs a zero-initialized ball vector.
   ///
   GameControllerBallVector()
   {
   }

   ///
   /// \brief Returns a pointer to the x component for SDL APIs.
   /// \return Pointer to x.
   ///
   float* getXPtr()
   {
      return &_x;
   }

   ///
   /// \brief Returns a pointer to the y component for SDL APIs.
   /// \return Pointer to y.
   ///
   float* getYPtr()
   {
      return &_y;
   }

protected:
   float _x = 0.0f;
   float _y = 0.0f;
};
