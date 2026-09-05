#include "menuscreen.h"

#include "framework/image/psd.h"
#include "framework/tools/localization.h"
#include "framework/tools/log.h"
#include "framework/tools/sfmlcompat.h"
#include "game/controller/gamecontrollerintegration.h"
#include "game/ui/menulabel.h"

#include <algorithm>
#include <cmath>
#include <iostream>

const sf::Color MenuScreen::color_label_normal{200, 185, 220};
const sf::Color MenuScreen::color_label_selected{255, 255, 255};
const sf::Color MenuScreen::color_help_text{130, 120, 150};

namespace
{

//! the colour every screen title is drawn in, taken from the artwork it replaces
const sf::Color color_title{165, 170, 237};

//! codepoint whose ink height is used as the vertical reference for every placed label
constexpr char32_t reference_codepoint{U'A'};

///
/// \brief Returns the y that vertically centers text inside reference_rect.
///
/// getLocalBounds() reports the ink the string actually covers, so a value carrying a descender
/// measures taller than one without, and centering on that would place the two at different
/// heights:
///
///     rect  +----------------+   +----------------+
///           |   Automatic    |   |  Power Saving  |   <- taller ink, so centering lifts it
///           +----------------+   +----------------+
///
/// The ink height of a capital is used instead. It is the same for every string, so cycling an
/// option leaves the text where it was.
///
/// \param text text whose character size and ink offset are read.
/// \param reference_rect rect to center in.
/// \return y position in whole pixels.
///
float centeredY(const sf::Text& text, const sf::FloatRect& reference_rect)
{
   const auto reference_height = getFont().getGlyph(reference_codepoint, text.getCharacterSize(), false, 0.0f).bounds.size.y;
   const auto ink_offset_y = text.getLocalBounds().position.y;
   return static_cast<float>(
      static_cast<int32_t>(reference_rect.position.y + (reference_rect.size.y - reference_height) / 2.0f - ink_offset_y)
   );
}

}  // namespace

MenuScreen::MenuScreen() : _font(getFont())
{
}

void MenuScreen::setTitle(const std::string& layer_name, const std::string& source_text, int32_t word_band_height_px)
{
   const auto& layer = _layers[layer_name];
   if (!layer || !layer->_texture || !layer->_sprite)
   {
      return;
   }

   const auto layer_size = layer->_texture->getSize();
   const auto width_px = static_cast<int32_t>(layer_size.x);
   const auto height_px = static_cast<int32_t>(layer_size.y);

   // a band cut to the height of a latin capital leaves a japanese glyph hanging into the ornament
   // below it, so a band that cannot hold one grows, and the layer moves up by as much as it grew.
   // that leaves the ornament on the pixel row it was drawn on
   constexpr auto padding_px = 2;
   const auto band_px = std::max(word_band_height_px, static_cast<int32_t>(title_character_size) + padding_px);
   const auto grown_px = band_px - word_band_height_px;

   const auto position = sfcompat::getPosition(*layer->_sprite);

   MenuLabel::compose(
      *layer,
      {width_px, height_px + grown_px},
      {MenuLabel::Piece{
         ._source = sf::IntRect{{0, word_band_height_px}, {width_px, height_px - word_band_height_px}},
         ._target = sf::Vector2i{0, band_px}
      }},
      {MenuLabel::Label{
         ._text = source_text,
         ._box = sf::IntRect{{0, 0}, {width_px, band_px}},
         ._align = MenuLabel::Align::Centered,
         ._character_size = title_character_size,
         ._color = color_title
      }}
   );

   sfcompat::setPosition(*layer->_sprite, {position.x, position.y - static_cast<float>(grown_px)});
}

void MenuScreen::setCaption(const std::string& layer_name, const std::string& source_text, const sf::Color& color)
{
   const auto& layer = _layers[layer_name];
   if (!layer || !layer->_texture || !layer->_sprite)
   {
      return;
   }

   constexpr auto caption_character_size = 12u;
   constexpr auto padding_px = 4;

   const auto layer_size = layer->_texture->getSize();
   const auto position = sfcompat::getPosition(*layer->_sprite);
   const auto center = position + sf::Vector2f{static_cast<float>(layer_size.x), static_cast<float>(layer_size.y)} / 2.0f;

   const auto width_px = std::max(1, static_cast<int32_t>(std::ceil(MenuLabel::measure(source_text, caption_character_size))));
   const auto height_px = std::max(static_cast<int32_t>(layer_size.y), static_cast<int32_t>(caption_character_size) + padding_px);

   MenuLabel::compose(
      *layer,
      {width_px, height_px},
      {},
      {MenuLabel::Label{
         ._text = source_text,
         ._box = sf::IntRect{{0, 0}, {width_px, height_px}},
         ._align = MenuLabel::Align::Centered,
         ._character_size = caption_character_size,
         ._color = color
      }}
   );

   sfcompat::setPosition(
      *layer->_sprite,
      {static_cast<float>(static_cast<int32_t>(center.x) - width_px / 2), static_cast<float>(static_cast<int32_t>(center.y) - height_px / 2)}
   );
}

void MenuScreen::placeTextCentered(sf::Text& text, const sf::FloatRect& reference_rect)
{
   const auto text_bounds = text.getLocalBounds();
   const auto x_px =
      static_cast<int32_t>(reference_rect.position.x + (reference_rect.size.x - text_bounds.size.x) / 2.0f - text_bounds.position.x);
   sfcompat::setPosition(text, {static_cast<float>(x_px), centeredY(text, reference_rect)});
}

void MenuScreen::placeTextLeft(sf::Text& text, const sf::FloatRect& reference_rect)
{
   const auto text_bounds = text.getLocalBounds();
   const auto x_px = static_cast<int32_t>(reference_rect.position.x - text_bounds.position.x);
   sfcompat::setPosition(text, {static_cast<float>(x_px), centeredY(text, reference_rect)});
}

void MenuScreen::placeTextRightOf(sf::Text& text, const sf::FloatRect& reference_rect)
{
   const auto text_bounds = text.getLocalBounds();
   const auto x_px =
      static_cast<int32_t>(reference_rect.position.x + reference_rect.size.x + button_text_x_offset - text_bounds.position.x);
   sfcompat::setPosition(text, {static_cast<float>(x_px), centeredY(text, reference_rect)});
}

void MenuScreen::placeDecorators(sf::Sprite& deco_left, sf::Sprite& deco_right, const sf::FloatRect& reference_rect)
{
   constexpr float decorator_gap_px = 10.0f;

   const auto deco_left_bounds = deco_left.getLocalBounds();
   const auto deco_left_x = static_cast<int32_t>(reference_rect.position.x - deco_left_bounds.size.x - decorator_gap_px);
   const auto deco_left_y = static_cast<int32_t>(reference_rect.position.y + (reference_rect.size.y - deco_left_bounds.size.y) / 2.0f);
   sfcompat::setPosition(deco_left, {static_cast<float>(deco_left_x), static_cast<float>(deco_left_y)});

   const auto deco_right_bounds = deco_right.getLocalBounds();
   const auto deco_right_x = static_cast<int32_t>(reference_rect.position.x + reference_rect.size.x + decorator_gap_px);
   const auto deco_right_y = static_cast<int32_t>(reference_rect.position.y + (reference_rect.size.y - deco_right_bounds.size.y) / 2.0f);
   sfcompat::setPosition(deco_right, {static_cast<float>(deco_right_x), static_cast<float>(deco_right_y)});
}

sf::FloatRect MenuScreen::rowRect(const sf::FloatRect& base_rect, int32_t row_index) const
{
   auto rect = base_rect;
   rect.position.y += static_cast<float>(row_index) * _row_stride;
   return rect;
}

void MenuScreen::placeDecorators(const sf::FloatRect& reference_rect)
{
   placeDecorators(*_layers["deco_l"]->_sprite, *_layers["deco_r"]->_sprite, reference_rect);
   _layers["deco_l"]->_visible = true;
   _layers["deco_r"]->_visible = true;
}

void MenuScreen::update(const sf::Time& /*dt*/)
{
}

void MenuScreen::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   for (auto& layer : _layer_stack)
   {
      if (layer->_visible)
      {
         layer->draw(window, states);
      }
   }
}

void MenuScreen::showEvent()
{
}

void MenuScreen::hideEvent()
{
}

const std::string& MenuScreen::getFilename()
{
   return _filename;
}

void MenuScreen::setFilename(const std::string& filename)
{
   _filename = filename;
}

void MenuScreen::load()
{
   PSD psd;
   psd.setColorFormat(PSD::ColorFormat::ABGR);
   psd.load(_filename);

   for (const auto& layer : psd.getLayers())
   {
      // skip groups
      if (!layer.isImageLayer())
      {
         continue;
      }

      auto tmp = std::make_shared<Layer>();

      try
      {
         const auto texture_size = sf::Vector2u(static_cast<uint32_t>(layer.getWidth()), static_cast<uint32_t>(layer.getHeight()));
#ifdef DECEPTUS_VRSFML
         auto texture = std::make_shared<sf::Texture>(std::move(*sf::Texture::create(texture_size)));
#else
         auto texture = std::make_shared<sf::Texture>(texture_size);
#endif
         auto opacity = layer.getOpacity();

         texture->update(reinterpret_cast<const uint8_t*>(layer.getImage().getData().data()));
#ifdef DECEPTUS_VRSFML
         auto sprite = std::make_shared<sf::Sprite>();

         sprite->position = {static_cast<float>(layer.getLeft()), static_cast<float>(layer.getTop())};
         sprite->color = sf::Color(255u, 255u, 255u, static_cast<uint8_t>(opacity));
         sprite->textureRect = sf::FloatRect{{0.f, 0.f}, {static_cast<float>(layer.getWidth()), static_cast<float>(layer.getHeight())}};
#else
         auto sprite = std::make_shared<sf::Sprite>(*texture);

         sprite->setPosition({static_cast<float>(layer.getLeft()), static_cast<float>(layer.getTop())});
         sprite->setColor(sf::Color(255u, 255u, 255u, static_cast<uint8_t>(opacity)));
#endif

         tmp->_texture = texture;
         tmp->_sprite = sprite;

         _layer_stack.push_back(tmp);
         _layers[layer.getName()] = tmp;
      }
      catch (...)
      {
         Log::Fatal() << "failed to create texture: " << layer.getName();
      }
   }

   loadingFinished();
}

void MenuScreen::loadingFinished()
{
}

void MenuScreen::keyboardKeyPressed(sf::Keyboard::Key /*key*/)
{
}

void MenuScreen::keyboardKeyReleased(sf::Keyboard::Key /*key*/)
{
}

void MenuScreen::controllerButtonX()
{
   // that's default behavior for most screens
   keyboardKeyPressed(sf::Keyboard::Key::D);
}

void MenuScreen::controllerButtonY()
{
}

bool MenuScreen::isControllerUsed() const
{
   return GameControllerIntegration::getInstance().isControllerConnected();
}
