#include "portal.h"

// game
#include "constants.h"
#include "player/player.h"
#include "fixturenode.h"

#include "framework/math/sfmlmath.h"
#include "framework/tmxparser/tmximage.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxpolyline.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxtileset.h"
#include "framework/tools/log.h"
#include "texturepool.h"

#include <atomic>
#include <iostream>


std::atomic<bool> Portal::_portal_lock = false;


//-----------------------------------------------------------------------------
Portal::Portal(GameNode* parent)
 : GameNode(parent)
{
   setName(typeid(Portal).name());
}


//-----------------------------------------------------------------------------
void Portal::draw(sf::RenderTarget& window, sf::RenderTarget& /*normal*/)
{
   // bump maps are not supported for now
   for (const auto& sprite : _sprites)
   {
      window.draw(sprite);
   }
}


//-----------------------------------------------------------------------------
sf::Vector2f Portal::getPortalPosition()
{
   const auto portal_pos = _sprites.at(_sprites.size()-1).getPosition();
   return portal_pos;
}



//-----------------------------------------------------------------------------
const sf::Vector2f& Portal::getTilePosition() const
{
   return _tile_positions;
}


//-----------------------------------------------------------------------------
void Portal::lock()
{
   _portal_lock = true;
}


//-----------------------------------------------------------------------------
void Portal::unlock()
{
   _portal_lock = false;
}


//-----------------------------------------------------------------------------
bool Portal::isLocked()
{
   return _portal_lock;
}


//-----------------------------------------------------------------------------
std::shared_ptr<Portal> Portal::getDestination() const
{
   return _destination;
}


//-----------------------------------------------------------------------------
void Portal::setDestination(const std::shared_ptr<Portal>& dst)
{
   _destination = dst;
}


//-----------------------------------------------------------------------------
void Portal::update(const sf::Time& /*dt*/)
{
   sf::Vector2f player_pos = Player::getCurrent()->getPixelPositionf();
   sf::Vector2f portal_pos = getPortalPosition();

   sf::Vector2f a(player_pos.x, player_pos.y);
   sf::Vector2f b(portal_pos.x + PIXELS_PER_TILE * 0.5f, portal_pos.y);

   float distance = SfmlMath::length(a - b);
   bool atPortal = (distance < PIXELS_PER_TILE * 1.0f);

   setPlayerAtPortal(atPortal);

   int i = 0;
   for (auto& sprite : _sprites)
   {
      sprite.setColor(
         sf::Color(
            255,
            255, // atPortal ? 150 : 255,
            255  // atPortal ? 150 : 255
         )
      );

      const auto x = static_cast<int>(_tile_positions.x);
      const auto y = static_cast<int>(_tile_positions.y);

      sprite.setPosition(
         sf::Vector2f(
            static_cast<float>(x * PIXELS_PER_TILE),
            static_cast<float>((i + y) * PIXELS_PER_TILE)
         )
      );

      i++;
   }
}


//-----------------------------------------------------------------------------
void Portal::link(
   std::vector<std::shared_ptr<GameMechanism>>& portals,
   TmxObject* tmx_object
)
{
   auto src_dst = tmx_object->_polyline->_polyline;

   sf::Vector2f src_f = src_dst.at(0);
   sf::Vector2f dst_f = src_dst.at(1);
   sf::Vector2i src(static_cast<int32_t>(floor(src_f.x)), static_cast<int32_t>(floor(src_f.y)));
   sf::Vector2i dst(static_cast<int32_t>(floor(dst_f.x)), static_cast<int32_t>(floor(dst_f.y)));

   const auto src_x = static_cast<int32_t>(src.x + tmx_object->_x_px) / PIXELS_PER_TILE;
   const auto src_y = static_cast<int32_t>(src.y + tmx_object->_y_px) / PIXELS_PER_TILE;
   const auto dst_x = static_cast<int32_t>(dst.x + tmx_object->_x_px) / PIXELS_PER_TILE;
   const auto dst_y = static_cast<int32_t>(dst.y + tmx_object->_y_px) / PIXELS_PER_TILE;

   std::shared_ptr<Portal> src_portal;
   std::shared_ptr<Portal> dst_portal;

   for (auto& p : portals)
   {
      auto portal = std::dynamic_pointer_cast<Portal>(p);
      sf::Vector2f portal_pos = portal->getPortalPosition();

      const auto px = static_cast<int32_t>(portal_pos.x / PIXELS_PER_TILE);
      const auto py = static_cast<int32_t>(portal_pos.y / PIXELS_PER_TILE);

      // todo: go to py..(py + mHeight)
      if (px == src_x && (py == src_y || py + 1 == src_y))
      {
         src_portal = portal;
      }

      if (px == dst_x && (py == dst_y || py + 1 == dst_y))
      {
         dst_portal = portal;
      }

      if (src_portal != nullptr && dst_portal != nullptr)
      {
         src_portal->_destination = dst_portal;
         break;
      }
   }

   // set the destination's destination to where we came from.
   // not sure if this is desired behavior. but for development purposes
   // it'll help :)
   if (!dst_portal)
   {
      Log::Error() << "please mark your dst portal correctly for id: " << tmx_object->_id;
   }

   if (!src_portal)
   {
      Log::Error() << "please mark your src portal correctly for id: " << tmx_object->_id;
   }

   dst_portal->_destination = src_portal;

   // Log::Info() << "src: " << srcPortal << " dst: " << dstPortal << " (" << tmxObject->mName << ")";
}


//-----------------------------------------------------------------------------
void Portal::addSprite(const sf::Sprite& sprite)
{
   _sprites.push_back(sprite);
}


//-----------------------------------------------------------------------------
bool Portal::isPlayerAtPortal() const
{
   return _player_at_portal;
}



//-----------------------------------------------------------------------------
void Portal::setPlayerAtPortal(bool playerAtPortal)
{
   _player_at_portal = playerAtPortal;
}


//-----------------------------------------------------------------------------
std::vector<std::shared_ptr<GameMechanism>> Portal::load(
   TmxLayer* layer,
   TmxTileSet* tileSet,
   const std::filesystem::path& basePath,
   const std::shared_ptr<b2World>&
)
{
   // Log::Info() << "load portal layer";

   if (!tileSet)
   {
      return {};
   }

   std::vector<std::shared_ptr<GameMechanism>> portals;

   sf::Vector2u tilesize = sf::Vector2u(tileSet->_tile_width_px, tileSet->_tile_height_px);
   const auto tiles    = layer->_data;
   const auto width    = layer->_width_px;
   const auto height   = layer->_height_px;
   const auto firstId  = tileSet->_first_gid;

   // populate the vertex array, with one quad per tile
   for (auto i = 0u; i < width; ++i)
   {
      for (auto j = 0u; j < height; ++j)
      {
         // get the current tile number
         int tileNumber = tiles[i + j * width];

         if (tileNumber != 0)
         {
            // find matching Portal
            std::shared_ptr<Portal> portal = nullptr;
            for (auto& p : portals)
            {
               auto tmp = std::dynamic_pointer_cast<Portal>(p);
               if (
                     static_cast<uint32_t>(tmp->_tile_positions.x) == i
                  && static_cast<uint32_t>(tmp->_tile_positions.y) + 1 == j )
               {
                  portal = tmp;
                  break;
               }
            }

            if (portal == nullptr)
            {
               portal = std::make_shared<Portal>();
               portals.push_back(portal);
               portal->_tile_positions.x = static_cast<float>(i);
               portal->_tile_positions.y = static_cast<float>(j);
               portal->_texture = TexturePool::getInstance().get((basePath / tileSet->_image->_source).string());

               if (layer->_properties != nullptr)
               {
                  portal->setZ(layer->_properties->_map["z"]->_value_int.value());
               }
            }

            portal->_height++;

            int tu = (tileNumber - firstId) % (portal->_texture->getSize().x / tilesize.x);
            int tv = (tileNumber - firstId) / (portal->_texture->getSize().x / tilesize.x);

            sf::Sprite sprite;
            sprite.setTexture(*portal->_texture);
            sprite.setTextureRect(
               sf::IntRect(
                  tu * PIXELS_PER_TILE,
                  tv * PIXELS_PER_TILE,
                  PIXELS_PER_TILE,
                  PIXELS_PER_TILE
               )
            );

            sprite.setPosition(
               sf::Vector2f(
                  static_cast<float>(i * PIXELS_PER_TILE),
                  static_cast<float>(j * PIXELS_PER_TILE)
               )
            );

            portal->addSprite(sprite);
         }
      }
   }

   return portals;
}


