#include "cameraroomlock.h"

#include "gameconfiguration.h"


bool CameraRoomLock::correctedCamera(float& x, float& y, float focus_offset, float view_ratio_y) const
{
   if (!_room)
   {
      return false;
   }

   //       +--------------->----+-----<--------------------------------+
   //       |               |    |     |                                |
   //       |               |    |     |                                |
   //       |               |    |     |                                |
   //       |               |    |     |                                |
   //       |               |    |     |                                |
   //       |               |    |     |                                |
   //       |               |    |     |                                |
   //       |               |    |     |                                |
   //       |               |    |     |                                |
   //       |               |    |     |                                |
   //       +--------------->----+-----<--------------------------------+--- y = player y + screen height / 1.5
   //       |               |    |     |                                |
   //       |               |    |     |                                |
   //       |               |    |     |                                |
   //       |               |    |     |                                |
   //       +--------------->----+-----<--------------------------------+
   //                       |    |     |
   //                       |    |     |
   //                    focus   |   focus
   //                    zone 0  |   zone 1
   //                            |
   //                            |
   //                          player x

   if (_room->_rects.empty())
   {
      return false;
   }

   // workflow (only for 'current' room)
   //
   // 1) check which in which rectangle the current camera center lies
   //    -> find the right FloatRect
   auto pos = sf::Vector2f{x, y};
   const auto rect_it =  _room->findRect(pos);
   if (rect_it == _room->_rects.end())
   {
      // that's an error.
      return false;
   }

   // 2) check if
   //    a) screen's right is within room bounds, assign if necessary
   //    b) screen's left is within room bounds, assign if necessary
   //    c) screen's top is within room bounds, assign if necessary
   //    d) screen's bottom is within room bounds, assign if necessary

   // need to incorporate the focus offset here because the player is not
   // necessarily in the middle of the screen but maybe a little more to the
   // left or to the right depending on its orientation
   const auto& config = GameConfiguration::getInstance();
   const auto half_width    = static_cast<float>(config._view_width / 2.0f);
   const auto height        = static_cast<float>(config._view_height);
   const auto height_top    = height * (1.0f - 1.0f / view_ratio_y);
   const auto height_bottom = height / view_ratio_y;

   const auto u = pos + sf::Vector2f{0.0f, -height_bottom};
   const auto d = pos + sf::Vector2f{0.0f,  height_top};
   const auto l = pos + sf::Vector2f{-half_width - focus_offset, 0.0f};
   const auto r = pos + sf::Vector2f{ half_width - focus_offset, 0.0f};

   auto corrected = false;


   // optimization could be to remember the last rect that was used

   // also to remember the rect created from the boundaries around the camera position

   const auto rect = *rect_it;
   if (!rect.contains(l))
   {
      // camera center is out of left boundary
      x = rect.left + half_width + focus_offset;
      corrected = true;
   }
   else if (!rect.contains(r))
   {
      // camera center is out of right boundary
      x = rect.left + rect.width - half_width + focus_offset;
      corrected = true;
   }

   if (!rect.contains(u))
   {
      // camera center is out of upper boundary
      y = rect.top + height_bottom;
      corrected = true;
   }
   else if (!rect.contains(d))
   {
      // camera center is out of lower boundary
      y = rect.top + rect.height - height_top;
      corrected = true;
   }

   return corrected;
}


void CameraRoomLock::setRoom(const std::shared_ptr<Room>& room)
{
   if (_room != room)
   {
      // Log::Info() << "reset room interpolation";
      _room = room;
   }
}


CameraRoomLock& CameraRoomLock::instance()
{
   static CameraRoomLock _instance;
   return _instance;
}
