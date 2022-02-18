#include "cameraroomlock.h"

#include "camerasystemconfiguration.h"
#include "gameconfiguration.h"


namespace
{
std::shared_ptr<Room> _room;
auto _locked_left   = false;
auto _locked_right  = false;
auto _locked_top    = false;
auto _locked_bottom = false;
}


bool CameraRoomLock::correctedCamera(float& x, float& y, float focus_offset)
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

   const auto rect = *rect_it;

   // 2) check if
   //    a) screen's right is within room bounds, assign if necessary
   //    b) screen's left is within room bounds, assign if necessary
   //    c) screen's top is within room bounds, assign if necessary
   //    d) screen's bottom is within room bounds, assign if necessary

   // need to incorporate the focus offset here because the player is not
   // necessarily in the middle of the screen but maybe a little more to the
   // left or to the right depending on its orientation
   const auto& game_config = GameConfiguration::getInstance();
   const auto& camera_config = CameraSystemConfiguration::getInstance();
   const auto half_width    = static_cast<float>(game_config._view_width / 2.0f);
   const auto height        = static_cast<float>(game_config._view_height);
   const auto height_top    = height * (1.0f - 1.0f / camera_config.getViewRatioY());
   const auto height_bottom = height / camera_config.getViewRatioY();

   const auto u = pos + sf::Vector2f{0.0f, -height_bottom};
   const auto d = pos + sf::Vector2f{0.0f,  height_top};
   const auto l = pos + sf::Vector2f{-half_width - focus_offset, 0.0f};
   const auto r = pos + sf::Vector2f{ half_width - focus_offset, 0.0f};

   _locked_left   = !rect.contains(l);
   _locked_right  = !rect.contains(r);
   _locked_top    = !rect.contains(u);
   _locked_bottom = !rect.contains(d);

   if (_locked_left)
   {
      // camera center is out of left boundary
      x = rect.left + half_width + focus_offset;
   }
   else if (_locked_right)
   {
      // camera center is out of right boundary
      x = rect.left + rect.width - half_width + focus_offset;
   }

   if (_locked_top)
   {
      // camera center is out of upper boundary
      y = rect.top + height_bottom;
   }
   else if (_locked_bottom)
   {
      // camera center is out of lower boundary
      y = rect.top + rect.height - height_top;
   }

   return _locked_left || _locked_right || _locked_top || _locked_bottom;
}


void CameraRoomLock::setRoom(const std::shared_ptr<Room>& room)
{
   if (_room != room)
   {
      // Log::Info() << "reset room interpolation";
      _room = room;
   }
}


void CameraRoomLock::readLockedSides(bool& left, bool& right, bool& top, bool& bottom)
{
   left   = _locked_left;
   right  = _locked_right;
   top    = _locked_top;
   bottom = _locked_bottom;
}
