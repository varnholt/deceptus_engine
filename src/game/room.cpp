#include "room.h"

#include <algorithm>
#include <iostream>
#include <sstream>

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


Room::Room(const sf::IntRect& rect)
{
   mRects.push_back(rect);
}


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


std::optional<Room> Room::find(const sf::Vector2i& p, const std::vector<Room>& rooms)
{
   const auto roomIt = std::find_if(rooms.begin(), rooms.end(), [p, rooms](const Room& r){
         const auto& it = r.findRect(p);
         return (it != r.mRects.end());
      }
   );

   if (roomIt == rooms.end())
   {
      return {};
   }

   return *roomIt;
}



void Room::deserialize(TmxObject* tmxObject, std::vector<Room>& rooms)
{
   // read key from tmx object
   std::istringstream f(tmxObject->mName);
   std::string key;
   if (!getline(f, key, '_'))
   {
      key = tmxObject->mName;
   }

   if (key.empty())
   {
      std::cerr << "ignoring unnamed room" << std::endl;
      return;
   }

   const auto it = std::find_if(rooms.begin(), rooms.end(), [key](const Room& r){
         return (r.mName == key);
      }
   );

   auto rect = sf::IntRect{
      static_cast<int32_t>(tmxObject->mX),
      static_cast<int32_t>(tmxObject->mY),
      static_cast<int32_t>(tmxObject->mWidth),
      static_cast<int32_t>(tmxObject->mHeight)
   };

   if (it == rooms.end())
   {
      // create new room
      Room room{rect};
      room.mName = key;
      rooms.push_back(room);

      std::cout << "adding room: " << key << std::endl;
   }
   else
   {
      // merge room
      auto& room = *it;

      // test for overlaps
      if (std::any_of(room.mRects.begin(), room.mRects.end(), [rect](const sf::IntRect& r){
               return r.intersects(rect);
            }
         )
      )
      {
         std::cerr << "bad rect intersection for room " << key << std::endl;
      }

      room.mRects.push_back(rect);
   }
}


