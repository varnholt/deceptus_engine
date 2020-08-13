#include "room.h"

#include <algorithm>


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


bool Room::contains(const sf::Vector2i& p) const
{
   const auto it = std::find_if(mRects.begin(),mRects.end(), [p](const sf::IntRect& rect){return rect.contains(p);});
   return it != mRects.end();
}


void Room::correctCamera(const sf::Vector2i& /*cameraCenter*/, const Room& /*activeRoom*/)
{
   // workflow (only for 'current' room)
   //
   // 1) check which in which rectangles the current camera center lies
   //    -> build a vector of intrects
   //
   //    well, ideally rooms shouldn't overlap but it might be worthwhile to
   //    support it anyway
   //
   // 2) go through each intrect and calculate the room for the camera center, i.e.
   //    - the minimum x (left)
   //    - the maximum x (right)
   //    - ...
   //
   // 3) check if
   //    a) screen's right is within room bounds, assign if necessary
   //    b) screen's left is within room bounds, assign if necessary
   //    c) screen's top is within room bounds, assign if necessary
   //    d) screen's bottom is within room bounds, assign if necessary
}


