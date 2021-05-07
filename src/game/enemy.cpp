#include "enemy.h"

#include "constants.h"
#include "framework/math/sfmlmath.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"

#include <iostream>
#include <sstream>

void Enemy::parse(TmxObject* object)
{
   mId = object->_id;
   mName = object->_name;
   mPixelPosition = {static_cast<int32_t>(object->_x_px), static_cast<int32_t>(object->_y_px)};

   if (object->_properties)
   {
      for (const auto& [k, v] : object->_properties->_map)
      {
         ScriptProperty property;
         property.mName = k;
         property.mValue = v->toString();
         mProperties.push_back(property);
      }
   }

   // add path property to path if exists
   const auto path = findProperty("path");
   if (path)
   {
      std::istringstream s(path.value().mValue);
      std::string field;
      while (std::getline(s, field, ','))
      {
         if (!field.empty())
         {
            mPixelPath.push_back(std::stoi(field) * PIXELS_PER_TILE);
         }
      }

      if (mPixelPath.size() == 2)
      {
         mPixelPath.insert(mPixelPath.begin(), mPixelPosition.y);
         mPixelPath.insert(mPixelPath.begin(), mPixelPosition.x);
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

   mRect.top = top;
   mRect.left = left;
   mRect.width = w;
   mRect.height = h;

   mVertices[0].x = left;
   mVertices[0].y = top;

   mVertices[1].x = left;
   mVertices[1].y = top  + h;

   mVertices[2].x = left + w;
   mVertices[2].y = top  + h;

   mVertices[3].x = left + w;
   mVertices[3].y = top;
}


void Enemy::addPaths(const std::vector<std::vector<b2Vec2>>& paths)
{
   // do destroy existing paths
   if (!mPath.empty())
   {
      return;
   }

   // a player rect can only overlap with a single chain.
   // this function finds this chain and assigns it.
   for (const auto& path : paths)
   {
      for (auto i0 = 0u; i0 < path.size(); i0++)
      {
         sf::Vector2i v0{
            static_cast<int32_t>(path[i0].x * PPM),
            static_cast<int32_t>(path[i0].y * PPM)
         };

         // check if rect contains point, then we have a match
         if (mRect.contains(v0))
         {
            mPath = path;
            mHasPath = true;
            break;
         }
         else
         {
            // otherwise check if the line intersects with the rect
            const auto i1 = (i0 == path.size() -1) ? 0u : (i0 + 1);

            sf::Vector2i v1{
               static_cast<int32_t>(path[i1].x * PPM),
               static_cast<int32_t>(path[i1].y * PPM)
            };

            const auto intersectsLeft   = SfmlMath::intersect(v0, v1, mVertices[0], mVertices[1]);
            const auto intersectsBottom = SfmlMath::intersect(v0, v1, mVertices[1], mVertices[2]);
            const auto intersectsRight  = SfmlMath::intersect(v0, v1, mVertices[2], mVertices[3]);
            const auto intersectsTop    = SfmlMath::intersect(v0, v1, mVertices[3], mVertices[0]);

            if (
                  intersectsLeft.has_value()
               || intersectsBottom.has_value()
               || intersectsRight.has_value()
               || intersectsTop.has_value()
            )
            {
               // std::cout << "assigned chain to: " << mId << std::endl;
               mPath = path;
               mHasPath = true;
               break;
            }
         }
      }
   }

   if (!mHasPath)
   {
      // not an error, enemy might just have a fixed position
      // std::cerr << "object " << mId << " (" << mName << ") has invalid chain" << std::endl;
   }
   else
   {
      for (const auto& v : mPath)
      {
         mPixelPath.push_back(static_cast<int32_t>(v.x * PPM));
         mPixelPath.push_back(static_cast<int32_t>(v.y * PPM));
      }
   }
}


std::optional<ScriptProperty> Enemy::findProperty(const std::string& key)
{
   std::optional<ScriptProperty> property;

   auto prop_it = std::find_if(mProperties.begin(), mProperties.end(), [key](auto& property){
      return property.mName == key;}
   );

   if (prop_it != mProperties.end())
   {
      return *prop_it;
   }

   return property;
}

