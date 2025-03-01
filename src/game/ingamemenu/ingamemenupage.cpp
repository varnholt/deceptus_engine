#include "game/ingamemenu/ingamemenupage.h"
#include "framework/easings/easings.h"
#include "framework/image/psd.h"
#include "framework/tools/log.h"
#include "game/config/gameconfiguration.h"
#include "game/state/displaymode.h"
#include "game/state/gamestate.h"

#include <iostream>

std::ostream& operator<<(std::ostream& os, InGameMenuPage::Animation animation)
{
   switch (animation)
   {
      case InGameMenuPage::Animation::Show:
      {
         os << "Show";
         break;
      }
      case InGameMenuPage::Animation::Hide:
      {
         os << "Hide";
         break;
      }
      case InGameMenuPage::Animation::MoveInFromLeft:
      {
         os << "MoveInFromLeft";
         break;
      }
      case InGameMenuPage::Animation::MoveOutToLeft:
      {
         os << "MoveOutToLeft";
         break;
      }
      case InGameMenuPage::Animation::MoveInFromRight:
      {
         os << "MoveInFromRight";
         break;
      }
      case InGameMenuPage::Animation::MoveOutToRight:
      {
         os << "MoveOutToRight";
         break;
      }
   }

   return os;
}

void InGameMenuPage::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   const auto w = GameConfiguration::getInstance()._view_width;
   const auto h = GameConfiguration::getInstance()._view_height;

   sf::View view(sf::FloatRect({0.0f, 0.0f}, {static_cast<float>(w), static_cast<float>(h)}));
   window.setView(view);

   for (auto& layer : _layer_stack)
   {
      if (layer->_visible)
      {
         layer->draw(window, states);
      }
   }
}

void InGameMenuPage::show()
{
   _animation = Animation::Show;
   _time_show = std::chrono::high_resolution_clock::now();
   update({});
}

void InGameMenuPage::hide()
{
   if (_animation.has_value())
   {
      return;
   }

   _animation = Animation::Hide;
   _time_hide = std::chrono::high_resolution_clock::now();
}

void InGameMenuPage::fullyHidden()
{
   GameState::getInstance().enqueueResume();
   DisplayMode::getInstance().enqueueUnset(Display::IngameMenu);
   _animation.reset();
}

std::optional<float> InGameMenuPage::getMoveOffset() const
{
   // move in from left:    -width .. 0
   // move in from right:    width .. 0
   // move out to left:     0 .. -width
   // move out to right:    0 ..  width

   const auto now = std::chrono::high_resolution_clock::now();
   const auto duration_since_move_start_s = now - _time_move;
   constexpr auto duration_move_s = 0.5f;

   if (duration_since_move_start_s.count() < duration_move_s)
   {
      const auto move_in = _animation == Animation::MoveInFromLeft || _animation == Animation::MoveInFromRight;
      const auto elapsed_s_normalized = duration_since_move_start_s.count() / duration_move_s;
      const auto val_normalized = (move_in) ? (1.0f - elapsed_s_normalized) : elapsed_s_normalized;
      const auto sign = (_animation == Animation::MoveInFromLeft || _animation == Animation::MoveOutToLeft) ? -1.0f : 1.0f;
      const auto val_eased = Easings::easeInOutCubic(val_normalized);
      const auto screen_width = GameConfiguration::getInstance()._view_width;
      const auto val = sign * val_eased * screen_width;

      // std::cout << _animation.value() << " val: " << val << std::endl;

      return val;
   }

   return std::nullopt;
}

void InGameMenuPage::debug()
{
}

std::optional<InGameMenuPage::Animation> InGameMenuPage::getAnimation() const
{
   return _animation;
}

void InGameMenuPage::moveOutToLeft()
{
   _time_move = std::chrono::high_resolution_clock::now();
   _animation = Animation::MoveOutToLeft;
}

void InGameMenuPage::moveInFromLeft()
{
   _time_move = std::chrono::high_resolution_clock::now();
   _animation = Animation::MoveInFromLeft;
}

void InGameMenuPage::moveOutToRight()
{
   _time_move = std::chrono::high_resolution_clock::now();
   _animation = Animation::MoveOutToRight;
}

void InGameMenuPage::moveInFromRight()
{
   _time_move = std::chrono::high_resolution_clock::now();
   _animation = Animation::MoveInFromRight;
}

void InGameMenuPage::load()
{
   if (_filename.empty())
   {
      return;
   }

   // load ingame psd
   PSD psd;
   psd.setColorFormat(PSD::ColorFormat::ABGR);
   psd.load(_filename);

   // std::cout << _filename << std::endl;

   for (const auto& layer : psd.getLayers())
   {
      // skip groups
      if (!layer.isImageLayer())
      {
         continue;
      }

      // std::cout << "add layer: " << layer.getName() << ": " << layer.getLeft() << ", " << layer.getTop() << " (" << layer.getWidth()
      //           << " x " << layer.getHeight() << ")" << std::endl;

      // make all layers visible per default, don't trust the PSD :)
      auto tmp = std::make_shared<Layer>();
      tmp->_visible = true;
      tmp->_name = layer.getName();

      auto texture = std::make_shared<sf::Texture>(static_cast<uint32_t>(layer.getWidth()), static_cast<uint32_t>(layer.getHeight()));
      auto sprite = std::make_shared<sf::Sprite>();

      texture->update(reinterpret_cast<const uint8_t*>(layer.getImage().getData().data()));

      sprite->setTexture(*texture, true);
      sprite->setPosition({static_cast<float>(layer.getLeft()), static_cast<float>(layer.getTop())});
      sprite->setColor(sf::Color{255, 255, 255, static_cast<uint8_t>(layer.getOpacity())});

      tmp->_texture = texture;
      tmp->_sprite = sprite;

      _layers[layer.getName()] = tmp;
      _layer_stack.push_back(tmp);
   }
}
