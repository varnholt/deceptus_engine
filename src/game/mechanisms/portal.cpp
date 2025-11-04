#include "portal.h"

// game
#include "framework/tmxparser/tmximage.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxpolyline.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxtileset.h"
#include "framework/tools/log.h"
#include "game/camera/camerasystem.h"
#include "game/constants.h"
#include "game/effects/fadetransitioneffect.h"
#include "game/effects/screentransition.h"
#include "game/io/texturepool.h"
#include "game/player/player.h"
#include "game/state/displaymode.h"

#include <atomic>
#include <iostream>

std::atomic<bool> Portal::_portal_lock = false;

Portal::Portal(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(Portal).name());
}

std::string_view Portal::objectName() const
{
   return "Portal";
}

void Portal::draw(sf::RenderTarget& window, sf::RenderTarget& /*normal*/)
{
   // bump maps are not supported for now
   for (const auto& sprite : _sprites)
   {
      window.draw(sprite);
   }
}

sf::Vector2f Portal::getPortalPosition()
{
   auto portal_pos = _sprites.at(_sprites.size() - 1).getPosition();
   portal_pos.y--;
   return portal_pos;
}

const sf::Vector2f& Portal::getTilePosition() const
{
   return _tile_positions;
}

void Portal::lock()
{
   _portal_lock = true;
}

void Portal::unlock()
{
   _portal_lock = false;
}

bool Portal::isLocked()
{
   return _portal_lock;
}

namespace
{
std::unique_ptr<ScreenTransition> makeFadeTransition()
{
   auto screen_transition = std::make_unique<ScreenTransition>();
   auto fade_out = std::make_shared<FadeTransitionEffect>();
   auto fade_in = std::make_shared<FadeTransitionEffect>();
   fade_out->_direction = FadeTransitionEffect::Direction::FadeOut;
   fade_out->_speed = 2.0f;
   fade_in->_direction = FadeTransitionEffect::Direction::FadeIn;
   fade_in->_value = 1.0f;
   fade_in->_speed = 2.0f;
   screen_transition->_effect_1 = fade_out;
   screen_transition->_effect_2 = fade_in;
   screen_transition->_delay_between_effects_ms = std::chrono::milliseconds{500};
   screen_transition->startEffect1();

   return screen_transition;
}

void goToPortal(auto portal)
{
   auto dst_position_px = portal->getDestination()->getPortalPosition();

   Player::getCurrent()->setBodyViaPixelPosition(
      dst_position_px.x + PLAYER_ACTUAL_WIDTH / 2, dst_position_px.y + DIFF_PLAYER_TILE_TO_PHYSICS
   );

   // update the camera system to point to the player position immediately
   CameraSystem::getInstance().syncNow();
   Portal::unlock();
}

}  // namespace

void Portal::use()
{
   if (DisplayMode::getInstance().isSet(Display::CameraPanorama))
   {
      return;
   }

   if (_portal_clock.getElapsedTime().asSeconds() > 1.0f)
   {
      if (Player::getCurrent()->getControls()->isButtonBPressed())
      {
         if (getDestination())
         {
            Portal::lock();
            _portal_clock.restart();

            auto screen_transition = makeFadeTransition();
            screen_transition->_callbacks_effect_1_ended.emplace_back([this]() { goToPortal(_destination); });
            screen_transition->_callbacks_effect_2_ended.emplace_back([]() { ScreenTransitionHandler::getInstance().pop(); });
            ScreenTransitionHandler::getInstance().push(std::move(screen_transition));
         }
      }
   }
}

std::shared_ptr<Portal> Portal::getDestination() const
{
   return _destination;
}

void Portal::setDestination(const std::shared_ptr<Portal>& dst)
{
   _destination = dst;
}

void Portal::update(const sf::Time& /*dt*/)
{
   const auto player_intersects = Player::getCurrent()->getPixelRectFloat().findIntersection(_rect).has_value();

   // activate portal when player intersects
   if (!_player_intersects && player_intersects)
   {
      _player_intersects = player_intersects;
      return;
   }

   // player uses gateway
   if (_player_intersects)
   {
      if (Player::getCurrent()->getControls()->isButtonBPressed())
      {
         use();
      }
   }

   sf::Vector2f player_pos = Player::getCurrent()->getPixelPositionFloat();
   sf::Vector2f portal_pos = getPortalPosition();

   sf::Vector2f a(player_pos.x, player_pos.y);
   sf::Vector2f b(portal_pos.x + PIXELS_PER_TILE * 0.5f, portal_pos.y);

   int32_t i = 0;
   for (auto& sprite : _sprites)
   {
      sprite.setColor(sf::Color(
         255,
         255,  // atPortal ? 150 : 255,
         255   // atPortal ? 150 : 255
      ));

      const auto x = static_cast<int32_t>(_tile_positions.x);
      const auto y = static_cast<int32_t>(_tile_positions.y);

      sprite.setPosition(sf::Vector2f(static_cast<float>(x * PIXELS_PER_TILE), static_cast<float>((i + y) * PIXELS_PER_TILE)));

      i++;
   }
}

std::optional<sf::FloatRect> Portal::getBoundingBoxPx()
{
   return _rect;
}

void Portal::link(std::vector<std::shared_ptr<GameMechanism>>& portals, const GameDeserializeData& data)
{
   if (!data._tmx_object->_polyline)
   {
      return;
   }

   auto src_dst = data._tmx_object->_polyline->_path;

   sf::Vector2f src_f = src_dst.at(0);
   sf::Vector2f dst_f = src_dst.at(1);
   sf::Vector2i src(static_cast<int32_t>(floor(src_f.x)), static_cast<int32_t>(floor(src_f.y)));
   sf::Vector2i dst(static_cast<int32_t>(floor(dst_f.x)), static_cast<int32_t>(floor(dst_f.y)));

   const auto src_x = static_cast<int32_t>(src.x + data._tmx_object->_x_px) / PIXELS_PER_TILE;
   const auto src_y = static_cast<int32_t>(src.y + data._tmx_object->_y_px) / PIXELS_PER_TILE;
   const auto dst_x = static_cast<int32_t>(dst.x + data._tmx_object->_x_px) / PIXELS_PER_TILE;
   const auto dst_y = static_cast<int32_t>(dst.y + data._tmx_object->_y_px) / PIXELS_PER_TILE;

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

      if (src_portal && dst_portal)
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
      Log::Error() << "please mark your dst portal correctly for id: " << data._tmx_object->_id;
   }

   if (!src_portal)
   {
      Log::Error() << "please mark your src portal correctly for id: " << data._tmx_object->_id;
   }

   dst_portal->_destination = src_portal;

   // Log::Info() << "src: " << srcPortal << " dst: " << dstPortal << " (" << tmxObject->mName << ")";
}

void Portal::addSprite(const sf::Sprite& sprite)
{
   _sprites.push_back(sprite);
}

std::vector<std::shared_ptr<GameMechanism>> Portal::load(GameNode* parent, const GameDeserializeData& data)
{
   // Log::Info() << "load portal layer";

   if (!data._tmx_tileset)
   {
      return {};
   }

   std::vector<std::shared_ptr<GameMechanism>> portals;

   sf::Vector2u tilesize = sf::Vector2u(data._tmx_tileset->_tile_width_px, data._tmx_tileset->_tile_height_px);
   const auto tiles = data._tmx_layer->_data;
   const auto width = data._tmx_layer->_width_tl;
   const auto height = data._tmx_layer->_height_tl;
   const auto firstId = data._tmx_tileset->_first_gid;

   // populate the vertex array, with one quad per tile
   for (auto i = 0u; i < width; ++i)
   {
      for (auto j = 0u; j < height; ++j)
      {
         // get the current tile number
         const auto tile_number = tiles[i + j * width];

         if (tile_number != 0)
         {
            // find matching portal
            std::shared_ptr<Portal> portal = nullptr;
            for (auto& p : portals)
            {
               auto tmp = std::dynamic_pointer_cast<Portal>(p);
               if (static_cast<uint32_t>(tmp->_tile_positions.x) == i && static_cast<uint32_t>(tmp->_tile_positions.y) + 1 == j)
               {
                  portal = tmp;
                  break;
               }
            }

            if (!portal)
            {
               portal = std::make_shared<Portal>(parent);
               portals.push_back(portal);
               portal->_tile_positions.x = static_cast<float>(i);
               portal->_tile_positions.y = static_cast<float>(j);
               portal->_texture = TexturePool::getInstance().get((data._base_path / data._tmx_tileset->_image->_source).string());
               portal->_rect = sf::FloatRect{{static_cast<float>(i), static_cast<float>(j)}, {PIXELS_PER_TILE, PIXELS_PER_TILE * 2}};

               if (data._tmx_layer->_properties)
               {
                  portal->setZ(data._tmx_layer->_properties->_map["z"]->_value_int.value());
               }
            }

            portal->_height++;

            const int32_t tu = (tile_number - firstId) % (portal->_texture->getSize().x / tilesize.x);
            const int32_t tv = (tile_number - firstId) / (portal->_texture->getSize().x / tilesize.x);

            sf::Sprite sprite(*portal->_texture);
            sprite.setTextureRect(sf::IntRect({tu * PIXELS_PER_TILE, tv * PIXELS_PER_TILE}, {PIXELS_PER_TILE, PIXELS_PER_TILE}));

            sprite.setPosition(sf::Vector2f(static_cast<float>(i * PIXELS_PER_TILE), static_cast<float>(j * PIXELS_PER_TILE)));

            portal->addSprite(sprite);
         }
      }
   }

   return portals;
}
