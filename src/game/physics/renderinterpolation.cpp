#include "renderinterpolation.h"

#include <algorithm>

namespace
{
//!< how far the frame being drawn sits past the last simulation step
float __alpha = 0.0f;

//!< how far past that the picture is aimed, in steps. see setLead
float __lead = 0.0f;

//!< whether the frame is drawn between two simulation states at all. see setEnabled
bool __enabled = true;
}  // namespace

namespace RenderInterpolation
{

void setAlpha(float alpha)
{
   __alpha = std::clamp(alpha, 0.0f, 1.0f);
}

float getAlpha()
{
   return __alpha;
}

void setLead(float lead)
{
   __lead = std::clamp(lead, 0.0f, 1.0f);
}

float getLead()
{
   return __lead;
}

void setEnabled(bool enabled)
{
   __enabled = enabled;
}

bool isEnabled()
{
   return __enabled;
}

sf::Vector2f positionPx(const sf::Vector2f& previous_px, const sf::Vector2f& current_px)
{
   return sf::Vector2f{valuePx(previous_px.x, current_px.x), valuePx(previous_px.y, current_px.y)};
}

float valuePx(float previous_px, float current_px)
{
   if (!__enabled)
   {
      return current_px;
   }

   return previous_px + (current_px - previous_px) * (__alpha + __lead);
}

}  // namespace RenderInterpolation

void InterpolatedPosition::step(float x_px, float y_px)
{
   _previous_px = _current_px;
   _current_px = sf::Vector2f{x_px, y_px};
}

sf::Vector2f InterpolatedPosition::getPositionPx() const
{
   return RenderInterpolation::positionPx(_previous_px, _current_px);
}
