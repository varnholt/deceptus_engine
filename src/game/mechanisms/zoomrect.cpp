#include "zoomrect.h"

#include <iostream>
#include "framework/tmxparser/tmxproperties.h"
#include "game/camera/camerazoom.h"
#include "game/io/valuereader.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"
#include "game/player/player.h"

namespace
{
const auto registered_zoomrect = []
{
   auto& registry = GameMechanismDeserializerRegistry::instance();
   registry.mapGroupToLayer("ZoomRect", "zoom_rects");

   registry.registerLayerName(
      "zoom_rects",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<ZoomRect>(parent);
         mechanism->setup(data);
         mechanisms["zoom_rects"]->push_back(mechanism);
      }
   );
   registry.registerObjectGroup(
      "ZoomRect",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<ZoomRect>(parent);
         mechanism->setup(data);
         mechanisms["zoom_rects"]->push_back(mechanism);
      }
   );
   return true;
}();
}  // namespace

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

// read "percentage1:zoom1;percentage2:zoom2;percentage3:zoom3;..."
std::vector<ZoomRect::ZoomFactor> parseZoomFactors(const std::string& zoom_factors)
{
   std::vector<ZoomRect::ZoomFactor> result;
   std::istringstream stream(zoom_factors);
   std::string pair;

   while (std::getline(stream, pair, ';'))
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

ZoomRect::ZoomRect(GameNode* parent) : GameNode(parent)
{
   static int32_t instance_counter{0};
   _instance_id = instance_counter++;
}

void ZoomRect::update(const sf::Time& dt)
{
   const auto& player_rect = Player::getCurrent()->getPixelRectFloat();
   const auto player_position_px = Player::getCurrent()->getPixelPositionFloat();
   const auto within_rect = (player_rect.findIntersection(_rect_px).has_value());

   if (!within_rect)
   {
      if (_within_rect_in_previous_frame)
      {
         _within_rect_in_previous_frame = false;
         CameraZoom::getInstance().setZoomFactor(1.0f);
      }
      return;
   }

   _within_rect_in_previous_frame = true;

   // get distance of player to rect center
   const auto radius_px = std::hypot(_center_px.x - player_position_px.x, _center_px.y - player_position_px.y);

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
   const auto radius_normalized = std::clamp(radius_px / _radius_px, 0.0f, 1.0f);

   // find most applicable inner radius
   auto factor = 0.0f;

   if (radius_normalized <= _zoom_factors.front()._radius)
   {
      factor = _zoom_factors.front()._factor;
   }

   if (radius_normalized >= _zoom_factors.back()._radius)
   {
      factor = _zoom_factors.back()._factor;
   }

   const auto upper = std::ranges::upper_bound(_zoom_factors, radius_normalized, {}, &ZoomFactor::_radius);
   const auto lower = upper - 1;
   const auto a = 1.0f - (radius_normalized - lower->_radius) / (upper->_radius - lower->_radius);
   factor = std::lerp(lower->_factor, upper->_factor, a);

   CameraZoom::getInstance().setZoomFactor(factor);

   // std::cout << a << " " << factor << std::endl;
}

void ZoomRect::setup(const GameDeserializeData& data)
{
   const auto x_px = data._tmx_object->_x_px;
   const auto y_px = data._tmx_object->_y_px;
   const auto width_px = data._tmx_object->_width_px;
   const auto height_px = data._tmx_object->_height_px;

   _rect_px = sf::FloatRect{{x_px, y_px}, {width_px, height_px}};
   _center_px = sf::Vector2f{x_px + width_px * 0.5f, y_px + height_px * 0.5f};
   _radius_px = std::hypot(_rect_px.size.x / 2.0f, _rect_px.size.y / 2.0f);

   if (data._tmx_object->_properties)
   {
      const auto& map = data._tmx_object->_properties->_map;
      const auto values = ValueReader::readValue<std::string>("values", map).value_or("0.0:1.0;1.0:1.0");
      _zoom_factors = parseZoomFactors(values);
      std::ranges::sort(_zoom_factors, [](const ZoomFactor& a, const ZoomFactor& b) { return a._radius < b._radius; });
   }
   else
   {
      _zoom_factors = {{0.0f, 1.0f}, {1.0, 1.0}};
   }
}

std::optional<sf::FloatRect> ZoomRect::getBoundingBoxPx()
{
   return _rect_px;
}
