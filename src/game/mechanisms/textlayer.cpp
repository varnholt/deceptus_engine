#include "textlayer.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "texturepool.h"

#include "game/bitmapfont.h"
#include "game/valuereader.h"

TextLayer::TextLayer(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(TextLayer).name());

   _bitmap_font.load("data/game/secret_font.png", "data/game/secret_font.map");
}

void TextLayer::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   _bitmap_font.draw(target, _bitmap_coords, static_cast<int32_t>(_rect.left), static_cast<int32_t>(_rect.top));
}

void TextLayer::update(const sf::Time& /*dt*/)
{
}

std::optional<sf::FloatRect> TextLayer::getBoundingBoxPx()
{
   return _rect;
}

std::shared_ptr<TextLayer> TextLayer::deserialize(GameNode* parent, const GameDeserializeData& data)
{
   std::shared_ptr<TextLayer> instance = std::make_shared<TextLayer>(parent);

   const auto& map = data._tmx_object->_properties->_map;

   const auto bounding_rect =
      sf::FloatRect{data._tmx_object->_x_px, data._tmx_object->_y_px, data._tmx_object->_width_px, data._tmx_object->_height_px};

   instance->setObjectId(data._tmx_object->_name);
   instance->_rect = bounding_rect;
   instance->addChunks(bounding_rect);
   instance->_z_index = ValueReader::readValue<int32_t>("z", map).value_or(0);
   instance->_text = ValueReader::readValue<std::string>("text", map).value_or("undefined");
   instance->_bitmap_coords = instance->_bitmap_font.getCoords(instance->_text);

   return instance;
}
