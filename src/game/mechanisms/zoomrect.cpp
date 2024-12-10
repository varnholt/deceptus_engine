#include "zoomrect.h"

#include "framework/tmxparser/tmxproperties.h"
#include "game/player/player.h"

ZoomRect::ZoomRect(GameNode* parent) : GameNode(parent)
{
}

void ZoomRect::update(const sf::Time& dt)
{
   const auto& player_rect = Player::getCurrent()->getPixelRectFloat();
   const auto within_rect = (player_rect.intersects(_rect_px));

   if (!within_rect)
   {
      // maybe if the last frame was within the rect, then now set the zoom factor to 1.0
      return;
   }

   // get distance of player to rect center

   //
   // +-------------------------------------------------------------
   // |
   // |
   // |
   // |
   // |
   // |                                          x
   // |                              x----------------------+
   // |                              |---___
   // |                              |      ---___ hyp
   // |                             y|            ---___    O
   // |                              |                  ---/|\
   // |                              +                     \ \
   // |
   // +------------------------------+-----------------------------
   //

   // normalize

   // find most applicable inner radius

   // apply according zoom factor
}

namespace
{
struct ZoomCircle
{
   sf::Vector2f center;                                // Center of the circle
   float radius;                                       // Total radius of the circle
   std::vector<std::pair<float, float>> zoom_factors;  // (percentage, zoom factor)
};
float getNormalizedDistance(const ZoomCircle& circle, const sf::Vector2f& player_position)
{
   float distance = std::hypot(player_position.x - circle.center.x, player_position.y - circle.center.y);
   return std::clamp(distance / circle.radius, 0.0f, 1.0f);  // Normalize to [0, 1]
}

std::vector<ZoomRect::ZoomFactor> parseZoomFactors(const std::string& zoom_factors)
{
   std::vector<ZoomRect::ZoomFactor> result;
   std::istringstream stream(zoom_factors);
   std::string pair;

   while (std::getline(stream, pair, ','))
   {
      std::istringstream pair_stream(pair);
      std::string percentage, zoom;

      if (std::getline(pair_stream, percentage, ':') && std::getline(pair_stream, zoom))
      {
         result.emplace_back(std::stof(percentage), std::stof(zoom));
      }
   }
   return result;
}

}  // namespace

void ZoomRect::setup(const GameDeserializeData& data)
{
   const auto x_px = data._tmx_object->_x_px;
   const auto y_px = data._tmx_object->_y_px;
   const auto width_px = data._tmx_object->_width_px;
   const auto height_px = data._tmx_object->_height_px;

   _rect_px = sf::FloatRect{x_px, y_px, width_px, height_px};
   _center_px = sf::Vector2f{x_px + width_px * 0.5f, y_px + height_px * 0.5f};
   _radius_px = std::max(width_px * 0.5f, height_px * 0.5f);

   data._tmx_object->_properties;
}
