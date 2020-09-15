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

   updateX();
   updateY();
}


void CameraSystem::updateX()
{
   auto player = Player::getCurrent();
   auto& config = CameraSystemConfiguration::getInstance();

   auto playerX = player->getPixelPositionf().x;
   auto playerY = player->getPixelPositionf().y;
   const auto corrected = mRoom->correctedCamera(playerX, playerY, mFocusOffset, config.getViewRatioY());

   const auto dx = (playerX - mX) / config.getDampingFactorX();
   const auto fCenter = mViewWidth / 2.0f;
   const auto fRange  = mViewWidth / config.getFocusZoneDivider();

   mFocusZoneX0 = fCenter - fRange;
   mFocusZoneX1 = fCenter + fRange;

   // shift focus zone based on player orientation
   const auto targetOffset =
      player->isPointingLeft()
         ? ( fRange * config.getTargetShiftFactor())
         : (-fRange * config.getTargetShiftFactor());

   const auto fcd = (targetOffset - mFocusOffset) / config.getDampingFactorX();
   if (fabs(mFocusOffset) < fabs(fRange * config.getTargetShiftFactor()))
   {
      mFocusOffset += fcd;
   }

   mFocusZoneX0 += mFocusOffset;
   mFocusZoneX1 += mFocusOffset;
   mFocusZoneCenter = ((mFocusZoneX0 + mFocusZoneX1) / 2.0f);

   // test if out of focus zone boundaries
   const auto test = playerX - mFocusZoneCenter;

   const auto f0 = mX - mFocusZoneX1;
   const auto f1 = mX - mFocusZoneX0;

   if (test < f0 || test > f1)
   {
      mFocusXTriggered = true;
   }

   // test if back within close boundaries
   else if (
         (test > mX - mFocusZoneCenter - config.getBackInBoundsToleranceX())
      && (test < mX - mFocusZoneCenter + config.getBackInBoundsToleranceX())
   )
   {
      mFocusXTriggered = false;
   }

   if (mFocusXTriggered || corrected)
   {
      mX += dx;
   }
}


void CameraSystem::updateY()
{
   auto& config = CameraSystemConfiguration::getInstance();

   const auto pRange  = mViewHeight / config.getPanicLineDivider();
   const auto pCenter = mViewHeight / 2.0f;

   mPanicLineY0 = pCenter - pRange;
   mPanicLineY1 = pCenter + pRange;

   const auto viewCenter = (mViewHeight / 2.0f);

   // test if out of panic line boundaries
   auto player = Player::getCurrent();

   auto playerX = player->getPixelPositionf().x;
   auto playerY = player->getPixelPositionf().y + config.getPlayerOffsetY();
   const auto corrected = mRoom->correctedCamera(playerX, playerY, mFocusOffset, config.getViewRatioY());

   const auto test = playerY - viewCenter;

   const auto p0 = mY - mPanicLineY1;
   const auto p1 = mY - mPanicLineY0;

   if ((test < p0 || test > p1) /*|| !player->isInAir()*/)
   {
      mFocusYTriggered = true;
   }

   // test if back within close boundaries
   else if (
         (test > mY - viewCenter - config.getBackInBoundsToleranceY())
      && (test < mY - viewCenter + config.getBackInBoundsToleranceY())
   )
   {
      mFocusYTriggered = false;
   }

   if (player->isInAir() && !mFocusYTriggered &&! corrected)
   {
      return;
   }

   const auto dy = (playerY - mY) / config.getDampingFactorY();
   mY += dy;
}


void CameraSystem::setRoom(const std::optional<Room>& room)
{
   if (mRoom->mId != room->mId)
   {
      mRoomInterpolation = 0.0f;
      std::cout << "[i] reset room interpolation" << std::endl;
   }

   mRoom = room;
}


float CameraSystem::getX() const
{
   // camera should be in the center of the focus zone
   return mX - mFocusZoneCenter;
}


float CameraSystem::getY() const
{
   return mY - (mViewHeight / CameraSystemConfiguration::getInstance().getViewRatioY());
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


