#ifndef CAMERAZOOM_H
#define CAMERAZOOM_H

#include <optional>
#include "SFML/Graphics.hpp"

/// \brief interpolates camera zoom and applies centered view-rectangle scaling.
class CameraZoom
{
public:
   /// \brief constructs the zoom controller with a neutral zoom factor.
   CameraZoom() = default;

   /// \brief advances the current zoom factor toward the active target zoom.
   /// \param dt elapsed frame time used for interpolation speed.
   void update(const sf::Time& dt);
   /// \brief sets a new target zoom factor when it differs from the current value.
   /// \param zoom_factor desired zoom multiplier.
   void setZoomFactor(float zoom_factor);
   /// \brief returns the requested zoom multiplier.
   /// \return target zoom factor when transitioning, otherwise 1.0f.
   float getZoomFactor() const;
   /// \brief rescales a view rectangle around its center using the current interpolated zoom.
   /// \param rect view rectangle to resize in place.
   void adjust(sf::FloatRect& rect);

   /// \brief returns the global camera zoom controller.
   /// \return singleton zoom instance.
   static CameraZoom& getInstance();

private:
   std::optional<float> _target_zoom_factor;
   float _current_zoom_factor{1.0f};
   float _zoom_speed{2.0f};
};

#endif  // CAMERAZOOM_H
