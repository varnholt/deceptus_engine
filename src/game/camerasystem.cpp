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


CameraSystem CameraSystem::sInstance;


void CameraSystem::update(float viewWidth, float viewHeight)
{
   mViewWidth = viewWidth;
   mViewHeight = viewHeight;

   auto player = Player::getPlayer(0);

   const auto playerX = player->getPixelPosition().x;
   const auto playerY = player->getPixelPosition().y;

   const auto dx = (playerX - mX) / 32.0f;
   const auto dy = (playerY - mY) / 16.0f;

   const auto fCenter = mViewWidth / 2.0f;
   const auto fRange  = mViewWidth / 6.0f;

   mFocusZoneX0 = fCenter - fRange;
   mFocusZoneX1 = fCenter + fRange;

   // shift focus zone by player orientation
   //
   // if (player->isPointingLeft())
   // {
   //    mFocusZoneX0 += fRange * 0.75f;
   //    mFocusZoneX1 += fRange * 0.75f;
   // }
   // else
   // {
   //    mFocusZoneX0 -= fRange * 0.75f;
   //    mFocusZoneX1 -= fRange * 0.75f;
   // }

   const auto pRange  = mViewHeight / 2.5f;
   const auto pCenter = mViewHeight / 2.0f;

   mPanicLineY0 = pCenter - pRange;
   mPanicLineY1 = pCenter + pRange;

   // test if out of bounds
   const auto test = playerX - ((mFocusZoneX0 + mFocusZoneX1) / 2.0f);

   const auto f0 = mX - mFocusZoneX1;
   const auto f1 = mX - mFocusZoneX0;

   if (test < f0 || test > f1)
   {
      mX += dx;
   }

   // std::cout << "test: " << test << " f0: " << f0 << " f1: " << f1 << std::endl;

   mY += dy;
}


float CameraSystem::getX() const
{
   // camera should be in the center of the focus zone
   return mX - ((mFocusZoneX0 + mFocusZoneX1) / 2.0f);
}


float CameraSystem::getY() const
{
   return mY - (mViewHeight / 1.5f);
}


float CameraSystem::getFocusZoneX0() const
{
   return mFocusZoneX0;
}


float CameraSystem::getFocusZoneX1() const
{
   return mFocusZoneX1;
}


CameraSystem& CameraSystem::getCameraSystem()
{
   return sInstance;
}

float CameraSystem::getPanicLineY0() const
{
   return mPanicLineY0;
}

float CameraSystem::getPanicLineY1() const
{
   return mPanicLineY1;
}


