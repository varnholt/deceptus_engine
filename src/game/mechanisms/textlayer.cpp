#include "textlayer.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxtools.h"
#include "framework/tools/log.h"
#include "game/io/texturepool.h"
#include "game/io/valuereader.h"
#include "game/layers/bitmapfont.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"

#include <array>

TextLayer::TextLayer(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(TextLayer).name());
}

std::string_view TextLayer::objectName() const
{
   return "TextLayer";
}

#ifdef DECEPTUS_VRSFML
void TextLayer::draw(sf::RenderTarget& target, sf::RenderTarget& normal)
{
   draw(target, normal, {});
}

void TextLayer::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/, const sf::RenderStates& states)
{
   if (_mode == Mode::Bitmap)
   {
      _bitmap_font.draw(
         target, _bitmap_coords, static_cast<int32_t>(_rect.position.x), static_cast<int32_t>(_rect.position.y), std::nullopt, states
      );
   }
   else if (_mode == Mode::TrueType)
   {
      target.draw(*_truetype_text, states);
   }
}
#else
void TextLayer::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   if (_mode == Mode::Bitmap)
   {
      _bitmap_font.draw(target, _bitmap_coords, static_cast<int32_t>(_rect.position.x), static_cast<int32_t>(_rect.position.y));
   }
   else if (_mode == Mode::TrueType)
   {
      target.draw(*_truetype_text);
   }
}
#endif

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
      sf::FloatRect{{data._tmx_object->_x_px, data._tmx_object->_y_px}, {data._tmx_object->_width_px, data._tmx_object->_height_px}};

   instance->setObjectId(data._tmx_object->_name);
   instance->_rect = bounding_rect;
   instance->addChunks(bounding_rect);
   instance->_z_index = ValueReader::readValue<int32_t>("z", map).value_or(0);
   instance->_text = ValueReader::readValue<std::string>("text", map).value_or("undefined");

   auto replace = [](std::string& str, const std::string& what, const std::string& with)
   {
      auto index = str.find(what, 0);
      while (index != std::string::npos)
      {
         str.replace(index, what.size(), with);
         index = str.find(what, index + with.size());
      }
   };

   replace(instance->_text, "<br>", "\n");

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
#ifdef DECEPTUS_VRSFML
      auto loaded_font = sf::Font::openFromFile(font_truetype.value());
      instance->_truetype_font = loaded_font.hasValue() ? std::optional{std::move(*loaded_font)} : std::nullopt;
      if (!instance->_truetype_font.has_value())
#else
      if (!instance->_truetype_font.openFromFile(font_truetype.value()))
#endif
      {
         Log::Error() << "failed to load font";
      }
      else
      {
         const auto font_size = ValueReader::readValue<int32_t>("truetype_font_size", map).value_or(12);
         const auto color = ValueReader::readValue<std::string>("truetype_font_color", map).value_or("#ffffffff");
         const auto rgba = TmxTools::color(color);

#ifdef DECEPTUS_VRSFML
         // vrsfml creates its glyph atlas smoothed and has no Font::setSmooth to clear it with, so
         // the texture is cleared directly. it holds one atlas of a fixed size that is never
         // reallocated, so the filter set here is the one it keeps
         instance->_truetype_font->getTexture().setSmooth(false);
         instance->_truetype_text = std::make_unique<sf::Text>(*instance->_truetype_font, sf::Text::Data{});
         instance->_truetype_text->position = {data._tmx_object->_x_px, data._tmx_object->_y_px};
         instance->_truetype_text->setString(instance->_text.c_str());
#else
         instance->_truetype_font.setSmooth(false);
         instance->_truetype_text = std::make_unique<sf::Text>(instance->_truetype_font);
         instance->_truetype_text->setPosition({data._tmx_object->_x_px, data._tmx_object->_y_px});
         instance->_truetype_text->setString(instance->_text);
#endif
         instance->_truetype_text->setCharacterSize(font_size);
         instance->_truetype_text->setFillColor({rgba[0], rgba[1], rgba[2], rgba[3]});
      }
   }

   return instance;
}

namespace
{
static constexpr std::array text_layer_properties{
   PropertyInfo{.name = "text", .type = "string", .default_value = std::string_view{"undefined"}, .required = true},
   PropertyInfo{.name = "bitmap_font_texture", .type = "string", .default_value = std::string_view{""}},
   PropertyInfo{.name = "bitmap_font_map", .type = "string", .default_value = std::string_view{""}},
   PropertyInfo{.name = "truetype_font", .type = "string", .default_value = std::string_view{""}},
   PropertyInfo{.name = "truetype_font_size", .type = "int", .default_value = int32_t{12}},
   PropertyInfo{.name = "truetype_font_color", .type = "string", .default_value = std::string_view{"#ffffffff"}},
   PropertyInfo{.name = "z", .type = "int", .default_value = int32_t{0}, .template_value = int32_t{20}},
};
static constexpr MechanismSchema text_layer_schema{
   .type_name = "TextLayer",
   .layer_name = "text_layers",
   .default_width = 192,
   .default_height = 48,
   .properties = text_layer_properties,
};
const auto registered_text_layer = []
{
   GameMechanismDeserializerRegistry::instance().registerSchema(text_layer_schema);
   return true;
}();
}  // namespace
