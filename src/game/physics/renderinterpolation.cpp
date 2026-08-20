#include "renderinterpolation.h"

#include <algorithm>
#include <cmath>

namespace
{
//!< how far the frame being drawn sits past the last simulation step
float __alpha = 0.0f;
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

sf::Vector2f positionPx(const sf::Vector2f& previous_px, const sf::Vector2f& current_px)
{
   return sf::Vector2f{valuePx(previous_px.x, current_px.x), valuePx(previous_px.y, current_px.y)};
}

float valuePx(float previous_px, float current_px)
{
   return std::round(previous_px + (current_px - previous_px) * __alpha);
}

}  // namespace RenderInterpolation
