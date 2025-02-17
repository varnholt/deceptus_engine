#include "camerazoom.h"

#include <limits>

void CameraZoom::update(const sf::Time& dt)
{
   if (!_target_zoom_factor.has_value())
   {
      return;
   }

   if (std::abs(_target_zoom_factor.value() - _current_zoom_factor) < std::numeric_limits<float>::epsilon())
   {
      _current_zoom_factor = _target_zoom_factor.value();
      _target_zoom_factor.reset();
      return;
   }

   const auto delta = _target_zoom_factor.value() - _current_zoom_factor;
   _current_zoom_factor += delta * dt.asSeconds() * _zoom_speed;
}

void CameraZoom::setZoomFactor(float zoom_factor)
{
   if (std::abs(zoom_factor - _current_zoom_factor) < std::numeric_limits<float>::epsilon())
   {
      return;
   }

   _target_zoom_factor = zoom_factor;
}

void CameraZoom::adjust(sf::FloatRect& rect)
{
   if (!_target_zoom_factor.has_value())
   {
      return;
   }

   const auto width = rect.size.x * _current_zoom_factor;
   const auto height = rect.size.y * _current_zoom_factor;

   const auto center_x = rect.position.x + rect.size.x * 0.5f;
   const auto center_y = rect.position.y + rect.size.y * 0.5f;

   rect.size.x = width;
   rect.size.y = height;
   rect.position.x = center_x - rect.size.x * 0.5f;
   rect.position.y = center_y - rect.size.y * 0.5f;
}

CameraZoom& CameraZoom::getInstance()
{
   static CameraZoom sInstance;
   return sInstance;
}

float CameraZoom::getZoomFactor() const
{
   return _target_zoom_factor.value_or(1.0f);
}
