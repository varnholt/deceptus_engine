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


Room::Room(const sf::FloatRect& rect)
{
   static int32_t sId = 0;

   mRects.push_back(rect);
   mId = sId;
   sId++;
}


std::vector<sf::FloatRect>::const_iterator Room::findRect(const sf::Vector2f& p) const
{
   const auto it = std::find_if(
         mRects.begin(),
         mRects.end(),
         [p](const sf::FloatRect& rect){
            return rect.contains(p);
         }
      );

   return it;
}


void Room::correctedCamera(float& x, float& y, float focusOffset, float viewRatioY) const
{

/*

      +--------------->----+-----<--------------------------------+
      |               |    |     |                                |
      |               |    |     |                                |
      |               |    |     |                                |
      |               |    |     |                                |
      |               |    |     |                                |
      |               |    |     |                                |
      |               |    |     |                                |
      |               |    |     |                                |
      |               |    |     |                                |
      |               |    |     |                                |
      +--------------->----+-----<--------------------------------+--- y = player y + screen height / 1.5
      |               |    |     |                                |
      |               |    |     |                                |
      |               |    |     |                                |
      |               |    |     |                                |
      +--------------->----+-----<--------------------------------+
                      |    |     |
                      |    |     |
                   focus   |   focus
                   zone 0  |   zone 1
                           |
                           |
                         player x
*/


   if (mRects.empty())
   {
      return;
   }

   // workflow (only for 'current' room)
   //
   // 1) check which in which rectangle the current camera center lies
   //    -> find the right FloatRect
   auto pos = sf::Vector2f{x, y};
   const auto rectIt =  findRect(pos);
   if (rectIt == mRects.end())
   {
      // that's an error.
      return;
   }

   // 2) check if
   //    a) screen's right is within room bounds, assign if necessary
   //    b) screen's left is within room bounds, assign if necessary
   //    c) screen's top is within room bounds, assign if necessary
   //    d) screen's bottom is within room bounds, assign if necessary

   const auto rect = *rectIt;

   const auto config = GameConfiguration::getInstance();

   // need to incorporate the focus offset here because the player is not
   // necessarily in the middle of the screen but maybe a little more to the
   // left or to the right depending on its orientation
   const auto halfWidth  = static_cast<float>(config.mViewWidth / 2.0f);
   const auto height = static_cast<float>(config.mViewHeight);

   const auto l = pos + sf::Vector2f{- halfWidth - focusOffset, 0.0f};
   const auto r = pos + sf::Vector2f{  halfWidth - focusOffset, 0.0f};

   const auto heightTop = height * (1.0f - 1.0f / viewRatioY);
   const auto heightBottom = height / viewRatioY;

   const auto u = pos + sf::Vector2f{0.0f, -heightBottom};
   const auto d = pos + sf::Vector2f{0.0f, heightTop};

   if (!rect.contains(l))
   {
      // camera center is out of left boundary
      x = rect.left + halfWidth + focusOffset;
   }
   else if (!rect.contains(r))
   {
      // camera center is out of right boundary
      x = rect.left + rect.width - halfWidth + focusOffset;
   }

   if (!rect.contains(u))
   {
      // camera center is out of upper boundary
      y = rect.top + heightBottom;
   }
   else if (!rect.contains(d))
   {
      // camera center is out of lower boundary
      y = rect.top + rect.height - heightTop;
   }
}


std::optional<Room> Room::computeCurrentRoom(const sf::Vector2f& cameraCenter, const std::vector<Room>& rooms) const
{
   return Room::find(cameraCenter, rooms);
}


std::optional<Room> Room::find(const sf::Vector2f& p, const std::vector<Room>& rooms)
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
   // ignore invalid rects
   const auto config = GameConfiguration::getInstance();
   if (tmxObject->mWidth < config.mViewWidth)
   {
      std::cerr << "[!] ignoring rect, room width smaller than screen width" << std::endl;
      return;
   }

   if (tmxObject->mHeight < config.mViewHeight)
   {
      std::cerr << "[!] ignoring rect, room height smaller than screen height" << std::endl;
      return;
   }

   // read key from tmx object
   std::istringstream f(tmxObject->mName);
   std::string key;
   if (!getline(f, key, '_'))
   {
      key = tmxObject->mName;
   }

   if (key.empty())
   {
      std::cerr << "[!] ignoring unnamed room" << std::endl;
      return;
   }

   auto rect = sf::FloatRect{
      tmxObject->mX,
      tmxObject->mY,
      tmxObject->mWidth,
      tmxObject->mHeight
   };

   // check if room already exists
   const auto it = std::find_if(rooms.begin(), rooms.end(), [key](const Room& r){
         return (r.mName == key);
      }
   );

   if (it == rooms.end())
   {
      // create new room
      Room room{rect};
      room.mName = key;
      rooms.push_back(room);

      std::cout << "[i] adding room: " << key << std::endl;
   }
   else
   {
      // merge room
      auto& room = *it;

      // test for overlaps
      if (std::any_of(room.mRects.begin(), room.mRects.end(), [rect](const sf::FloatRect& r){
               return r.intersects(rect);
            }
         )
      )
      {
         std::cerr << "[!] bad rect intersection for room " << key << std::endl;
      }

      room.mRects.push_back(rect);
   }
}


