#include "camerasystem.h"

#include "player.h"

#include <iostream>


/*

   Camera system

      Camera X position

         1) dist = distance between old camera x position and the current x player position
         2) dist /= 32
         3) dist = min(dist, max_camera_speed)
         4) have a focus zone which is a rectangle around the player
            if the player walks out of the focus zone, apply the offset
         5) that rectangle is moved further to the left if the player looks to the right and vice versa.
         6) if the player accelerates in a direction that rectangle is moved even further into the opposite direction.


      Camera Y position

         1) dist = distance between old camera y position and the current player y position
         2) dist /= 16
         3) the camera is only moved when the player touches the ground
         4) have two 'panic lines' at the top and bottom of the screen; if the player falls through those lines,
            move the camera


              f0          f1
               |           |
      ---------+-----------+--- p0
               |           |
               |    O      |
               |   `|`     |
               |    |\     |
               |    ###    |
               |    ###    |
      ---------+-----------+--- p1
      ###   ###|###########|###
      ###   ###|###########|###

*/


void CameraSystem::update(float viewWidth, float viewHeight)
{
   mViewWidth = viewWidth;
   mViewHeight = viewHeight;

   const auto playerX = Player::getPlayer(0)->getPixelPosition().x;
   const auto playerY = Player::getPlayer(0)->getPixelPosition().y;

   const auto dx = (playerX - mX) / 32.0f;
   const auto dy = (playerY - mY) / 16.0f;

   const auto fRange = mViewWidth / 6.0f;
   const auto fCenter = (mViewWidth / 2.0f);
   [[maybe_unused]] const auto f0 = fCenter - fRange;
   [[maybe_unused]] const auto f1 = fCenter + fRange;

   // test if out of bounds
   // const auto tx = mX + dx - fCenter;
   // if (tx < f0 || tx > f1)
   // {
   // }
   //
   // std::cout << "f0: " << f0 << " f1: " << f1 << " tx: " << tx << std::endl;

   mX += dx;
   mY += dy;
}


float CameraSystem::getX() const
{
   return mX - (mViewWidth / 2.0f);
}


float CameraSystem::getY() const
{
   return mY - (mViewHeight / 1.5f);
}


