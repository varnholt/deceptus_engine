#ifndef CAMERAZOOM_H
#define CAMERAZOOM_H

#include <optional>
#include "SFML/Graphics.hpp"

class CameraZoom
{
public:
   CameraZoom() = default;

   void update(const sf::Time& dt);
   void setZoomFactor(float zoom_factor);
   float getZoomFactor() const;
   void adjust(sf::FloatRect&);

   static CameraZoom& getInstance();

private:
   std::optional<float> _target_zoom_factor;
   float _current_zoom_factor{1.0f};
   float _zoom_speed{2.0f};
};

#endif  // CAMERAZOOM_H
