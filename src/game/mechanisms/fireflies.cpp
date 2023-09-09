#include "fireflies.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "texturepool.h"

Fireflies::Fireflies(GameNode* parent) : GameNode(parent)
{
}

void Fireflies::draw(sf::RenderTarget& target, sf::RenderTarget& normal)
{
}

void Fireflies::update(const sf::Time& dt)
{
   for (auto& firefly : _fireflies)
   {
      firefly.update(dt);
   }
}

std::optional<sf::FloatRect> Fireflies::getBoundingBoxPx()
{
   return std::nullopt;
}

void Fireflies::deserialize(const GameDeserializeData& data)
{
   setObjectId(data._tmx_object->_name);

   auto count = 1;
   if (data._tmx_object->_properties)
   {
      auto it = data._tmx_object->_properties->_map.find("z");
      if (it != data._tmx_object->_properties->_map.end())
      {
         _z_index = it->second->_value_int.value();
      }

      it = data._tmx_object->_properties->_map.find("count");
      if (it != data._tmx_object->_properties->_map.end())
      {
         count = it->second->_value_int.value();
      }
   }

   _texture = TexturePool::getInstance().get("data/sprites/fireflies.png");

   for (auto i = 0; i < count; i++)
   {
      _fireflies.push_back({});
   }

   for (auto& firefly : _fireflies)
   {
      firefly._sprite.setTexture(*_texture);
   }
}

void Fireflies::Firefly::update(const sf::Time& dt)
{
}
