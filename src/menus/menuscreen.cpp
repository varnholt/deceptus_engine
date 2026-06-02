#include "menuscreen.h"

#include "framework/image/psd.h"
#include "framework/tools/localization.h"
#include "framework/tools/log.h"
#include "game/controller/gamecontrollerintegration.h"

#include <iostream>

const sf::Color MenuScreen::color_label_normal{200, 185, 220};
const sf::Color MenuScreen::color_label_selected{255, 255, 255};
const sf::Color MenuScreen::color_help_text{130, 120, 150};

MenuScreen::MenuScreen() : _font(getFont())
{
}

void MenuScreen::placeTextCentered(sf::Text& text, const sf::FloatRect& reference_rect)
{
   const auto text_bounds = text.getLocalBounds();
   const auto pixel_x =
      static_cast<int32_t>(reference_rect.position.x + (reference_rect.size.x - text_bounds.size.x) / 2.0f - text_bounds.position.x);
   const auto pixel_y =
      static_cast<int32_t>(reference_rect.position.y + (reference_rect.size.y - text_bounds.size.y) / 2.0f - text_bounds.position.y);
   text.setPosition({static_cast<float>(pixel_x), static_cast<float>(pixel_y)});
}

void MenuScreen::placeTextLeft(sf::Text& text, const sf::FloatRect& reference_rect)
{
   const auto text_bounds = text.getLocalBounds();
   const auto pixel_x = static_cast<int32_t>(reference_rect.position.x - text_bounds.position.x);
   const auto pixel_y =
      static_cast<int32_t>(reference_rect.position.y + (reference_rect.size.y - text_bounds.size.y) / 2.0f - text_bounds.position.y);
   text.setPosition({static_cast<float>(pixel_x), static_cast<float>(pixel_y)});
}

void MenuScreen::placeTextRightOf(sf::Text& text, const sf::FloatRect& reference_rect)
{
   const auto text_bounds = text.getLocalBounds();
   const auto pixel_x =
      static_cast<int32_t>(reference_rect.position.x + reference_rect.size.x + button_text_x_offset - text_bounds.position.x);
   const auto pixel_y =
      static_cast<int32_t>(reference_rect.position.y + (reference_rect.size.y - text_bounds.size.y) / 2.0f - text_bounds.position.y);
   text.setPosition({static_cast<float>(pixel_x), static_cast<float>(pixel_y)});
}

void MenuScreen::placeDecorators(sf::Sprite& deco_left, sf::Sprite& deco_right, const sf::FloatRect& reference_rect)
{
   constexpr float decorator_gap_px = 10.0f;

   const auto deco_left_bounds = deco_left.getLocalBounds();
   const auto deco_left_x = static_cast<int32_t>(reference_rect.position.x - deco_left_bounds.size.x - decorator_gap_px);
   const auto deco_left_y = static_cast<int32_t>(reference_rect.position.y + (reference_rect.size.y - deco_left_bounds.size.y) / 2.0f);
   deco_left.setPosition({static_cast<float>(deco_left_x), static_cast<float>(deco_left_y)});

   const auto deco_right_bounds = deco_right.getLocalBounds();
   const auto deco_right_x = static_cast<int32_t>(reference_rect.position.x + reference_rect.size.x + decorator_gap_px);
   const auto deco_right_y = static_cast<int32_t>(reference_rect.position.y + (reference_rect.size.y - deco_right_bounds.size.y) / 2.0f);
   deco_right.setPosition({static_cast<float>(deco_right_x), static_cast<float>(deco_right_y)});
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
         auto texture = std::make_shared<sf::Texture>(texture_size);
         auto opacity = layer.getOpacity();

         texture->update(reinterpret_cast<const uint8_t*>(layer.getImage().getData().data()));
         auto sprite = std::make_shared<sf::Sprite>(*texture);

         sprite->setPosition({static_cast<float>(layer.getLeft()), static_cast<float>(layer.getTop())});
         sprite->setColor(sf::Color(255u, 255u, 255u, static_cast<uint8_t>(opacity)));

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
