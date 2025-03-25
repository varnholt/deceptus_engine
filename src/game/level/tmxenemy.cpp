#include "tmxenemy.h"

#include "constants.h"
#include "framework/math/sfmlmath.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"

#include <iostream>
#include <ranges>
#include <sstream>

void TmxEnemy::parse(const std::shared_ptr<TmxObject>& object)
{
   _id = object->_id;
   _name = object->_name;
   _pixel_position = {static_cast<int32_t>(object->_x_px), static_cast<int32_t>(object->_y_px)};

   if (object->_properties)
   {
      for (const auto& [k, v] : object->_properties->_map)
      {
         ScriptProperty property;
         property._name = k;
         property._value = v->toString();
         _properties.push_back(property);
      }
   }

   // add path property to path if exists
   const auto path = findProperty("path");
   if (path)
   {
      std::istringstream s(path.value()._value);
      std::string field;
      while (std::getline(s, field, ','))
      {
         if (!field.empty())
         {
            _pixel_path.push_back(std::stoi(field) * PIXELS_PER_TILE);
         }
      }

      // if the path just consists of a single position, add another position which is the
      // pixel position of the tmx object
      if (_pixel_path.size() == 2)
      {
         _pixel_path.insert(_pixel_path.begin(), _pixel_position.y);
         _pixel_path.insert(_pixel_path.begin(), _pixel_position.x);
      }
   }

   auto w = static_cast<int32_t>(object->_width_px);
   auto h = static_cast<int32_t>(object->_height_px);

   if (w == 0)
   {
      w = PIXELS_PER_TILE;
   }

   if (h == 0)
   {
      h = PIXELS_PER_TILE;
   }

   auto left = static_cast<int32_t>(object->_x_px);
   auto top = static_cast<int32_t>(object->_y_px);

   top -= PIXELS_PER_TILE / 2;
   left -= PIXELS_PER_TILE / 2;

   _pixel_rect.position.y = top;
   _pixel_rect.position.x = left;
   _pixel_rect.size.x = w;
   _pixel_rect.size.y = h;

   _vertices[0].x = left;
   _vertices[0].y = top;

   _vertices[1].x = left;
   _vertices[1].y = top + h;

   _vertices[2].x = left + w;
   _vertices[2].y = top + h;

   _vertices[3].x = left + w;
   _vertices[3].y = top;
}

void TmxEnemy::addPaths(const std::vector<std::vector<b2Vec2>>& paths)
{
   // do not destroy existing paths
   if (!_path.empty())
   {
      return;
   }

   // a player rect can only overlap with a single chain.
   // this function finds this chain and assigns it.
   for (const auto& path : paths)
   {
      for (auto i0 = 0u; i0 < path.size(); i0++)
      {
         sf::Vector2i v0{static_cast<int32_t>(path[i0].x * PPM), static_cast<int32_t>(path[i0].y * PPM)};

         // check if rect contains point, then we have a match
         if (_pixel_rect.contains(v0))
         {
            _path = path;
            _has_path = true;
            break;
         }
         else
         {
            // otherwise check if the line intersects with the rect
            const auto i1 = (i0 == path.size() - 1) ? 0u : (i0 + 1);

            sf::Vector2i v1{static_cast<int32_t>(path[i1].x * PPM), static_cast<int32_t>(path[i1].y * PPM)};

            const auto intersects_left = SfmlMath::intersect(v0, v1, _vertices[0], _vertices[1]);
            const auto intersects_bottom = SfmlMath::intersect(v0, v1, _vertices[1], _vertices[2]);
            const auto intersects_right = SfmlMath::intersect(v0, v1, _vertices[2], _vertices[3]);
            const auto intersects_top = SfmlMath::intersect(v0, v1, _vertices[3], _vertices[0]);

            if (intersects_left.has_value() || intersects_bottom.has_value() || intersects_right.has_value() || intersects_top.has_value())
            {
               // Log::Info() << "assigned chain to: " << mId;
               _path = path;
               _has_path = true;
               break;
            }
         }
      }
   }

   if (_has_path)
   {
      // path inversion depends on level winding
      if (_winding == Winding::Clockwise)
      {
         _inverse_path = !_inverse_path;
      }

      if (_inverse_path)
      {
         for (const auto& v : std::ranges::views::reverse(_path))
         {
            _pixel_path.push_back(static_cast<int32_t>(v.x * PPM));
            _pixel_path.push_back(static_cast<int32_t>(v.y * PPM));
         }
      }
      else
      {
         for (const auto& v : _path)
         {
            _pixel_path.push_back(static_cast<int32_t>(v.x * PPM));
            _pixel_path.push_back(static_cast<int32_t>(v.y * PPM));
         }
      }
   }
}

std::optional<ScriptProperty> TmxEnemy::findProperty(const std::string& key)
{
   std::optional<ScriptProperty> property;

   auto prop_it = std::find_if(_properties.begin(), _properties.end(), [key](const auto& property) { return property._name == key; });

   if (prop_it != _properties.end())
   {
      return *prop_it;
   }

   return property;
}
