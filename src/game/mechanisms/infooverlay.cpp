#include "infooverlay.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "texturepool.h"

InfoOverlay::InfoOverlay(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(InfoOverlay).name());
   setEnabled(false);
}

void InfoOverlay::update(const sf::Time& dt)
{
   if (!isEnabled())
   {
      return;
   }

   _elapsed += dt;
}

void InfoOverlay::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   color.draw(_sprite);
}

std::shared_ptr<InfoOverlay> InfoOverlay::setup(GameNode* parent, const GameDeserializeData& data)
{
   auto instance = std::make_shared<InfoOverlay>(parent);

   const auto bounding_rect =
      sf::FloatRect{data._tmx_object->_x_px, data._tmx_object->_y_px, data._tmx_object->_width_px, data._tmx_object->_height_px};

   instance->setObjectId(data._tmx_object->_name);
   instance->_rect = bounding_rect;
   instance->_sprite.setPosition(data._tmx_object->_x_px, data._tmx_object->_y_px);
   instance->addChunks(bounding_rect);

   if (data._tmx_object->_properties)
   {
      auto z = data._tmx_object->_properties->_map.find("z");
      if (z != data._tmx_object->_properties->_map.end())
      {
         instance->_z_index = z->second->_value_int.value();
      }

      auto texture_id = data._tmx_object->_properties->_map.find("texture");
      if (texture_id != data._tmx_object->_properties->_map.end())
      {
         instance->_texture = TexturePool::getInstance().get(texture_id->second->_value_string.value());
      }
   }

   return instance;
}

std::optional<sf::FloatRect> InfoOverlay::getBoundingBoxPx()
{
   return std::nullopt;
}
