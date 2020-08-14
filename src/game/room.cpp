#include "room.h"

#include <algorithm>
#include "gameconfiguration.h"

/*

   room a_0
   +--------------------------------------------------------------------------+
   |                                                                          |
   |  screen                                                                  |
   |  +---------------/.\----------------+                                    |
   |  |                |                 |                                    |
   |  |                |                 |                                    |
   |  |                |                 |                                    |
   |  <----------------+----------------->                                    |
   |  |                |                 |                                    |
   |  |                |                 |                                    |
   |  |                |                 |                                    |
   |  +---------------\./----------------+                                    |
   |                                                                          |
   |                                    room a_1                              |
   +------------------------------------+-------------------------------------+
                                        |                                     |
                                        |                                     |
                                        |                                     |
                                        |                                     |
                                        |                                     |
                                        |                                     |
                                        |                                     |
                                        |                                     |
                                        |                                     |
                                        +-------------------------------------+

*/


Room::Room(const std::vector<sf::IntRect>& rect)
 : mRects(rect)
{
}


std::vector<sf::IntRect>::const_iterator Room::findRect(const sf::Vector2i& p) const
{
   const auto it = std::find_if(mRects.begin(),mRects.end(), [p](const sf::IntRect& rect){return rect.contains(p);});
   return it;
}


std::optional<sf::Vector2i> Room::correctedCamera(const sf::Vector2i& cameraCenter, const Room& /*activeRoom*/)
{
   // workflow (only for 'current' room)
   //
   // 1) check which in which rectangle the current camera center lies
   //    -> find the right intrect
   const auto rectIt =  findRect(cameraCenter);
   if (rectIt == mRects.end())
   {
      // that's an error.
      return {};
   }

   // 2) check if
   //    a) screen's right is within room bounds, assign if necessary
   //    b) screen's left is within room bounds, assign if necessary
   //    c) screen's top is within room bounds, assign if necessary
   //    d) screen's bottom is within room bounds, assign if necessary

   const auto rect = *rectIt;

   const auto config = GameConfiguration::getInstance();
   const auto halfWidth = config.mViewWidth / 2;
   const auto halfHeight = config.mViewHeight / 2;

   const auto l = cameraCenter + sf::Vector2i{- halfWidth, 0};
   const auto r = cameraCenter + sf::Vector2i{  halfWidth, 0};
   const auto u = cameraCenter + sf::Vector2i{0, - halfHeight};
   const auto d = cameraCenter + sf::Vector2i{0,   halfHeight};

   bool corrected = false;
   std::optional<sf::Vector2i> correctedRect = cameraCenter;

   if (!rect.contains(l))
   {
      // camera center is out of left boundary
      correctedRect->x = l.x + halfWidth;
      corrected = true;
   }
   else if (!rect.contains(r))
   {
      // camera center is out of right boundary
      correctedRect->x = r.x - halfWidth;
      corrected = true;
   }

   if (!rect.contains(u))
   {
      // camera center is out of upper boundary
      correctedRect->y = u.y + halfHeight;
      corrected = true;
   }
   else if (!rect.contains(d))
   {
      // camera center is out of lower boundary
      correctedRect->y = d.y - halfHeight;
      corrected = true;
   }

   return corrected ? correctedRect : std::optional<sf::Vector2i>{};
}


