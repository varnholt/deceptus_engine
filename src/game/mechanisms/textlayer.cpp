#include "textlayer.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxtools.h"
#include "framework/tools/log.h"
#include "game/bitmapfont.h"
#include "game/texturepool.h"
#include "game/valuereader.h"

TextLayer::TextLayer(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(TextLayer).name());
}

void TextLayer::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   if (_mode == Mode::Bitmap)
   {
      _bitmap_font.draw(target, _bitmap_coords, static_cast<int32_t>(_rect.left), static_cast<int32_t>(_rect.top));
   }
   else if (_mode == Mode::TrueType)
   {
      target.draw(_truetype_text);
   }
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

   // handle bitmap font
   const auto font_bitmap = ValueReader::readValue<std::string>("bitmap_font_texture", map);
   const auto font_map = ValueReader::readValue<std::string>("bitmap_font_map", map);
   if (font_bitmap.has_value() && font_map.has_value())
   {
      instance->_mode = Mode::Bitmap;
      instance->_bitmap_font.load(font_bitmap.value(), font_map.value());
      instance->_bitmap_coords = instance->_bitmap_font.getCoords(instance->_text);
   }

   // handle truetype font
   const auto font_truetype = ValueReader::readValue<std::string>("truetype_font", map);
   if (font_truetype.has_value())
   {
      instance->_mode = Mode::TrueType;
      if (!instance->_truetype_font.loadFromFile(font_truetype.value()))
      {
         Log::Error() << "failed to load font";
      }
      else
      {
         instance->_truetype_font.setSmooth(false);
         instance->_truetype_text.setPosition(data._tmx_object->_x_px, data._tmx_object->_y_px);
         instance->_truetype_text.setString(instance->_text);

         const auto font_size = ValueReader::readValue<int32_t>("truetype_font_size", map).value_or(12);
         instance->_truetype_text.setCharacterSize(font_size);
         instance->_truetype_text.setFont(instance->_truetype_font);

         const auto color = ValueReader::readValue<std::string>("truetype_font_color", map).value_or("#ffffffff");
         const auto rgba = TmxTools::color(color);
         instance->_truetype_text.setFillColor({rgba[0], rgba[1], rgba[2], rgba[3]});
      }
   }

   return instance;
}
