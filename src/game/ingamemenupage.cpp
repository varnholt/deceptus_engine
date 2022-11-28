#include "ingamemenupage.h"

#include "framework/image/psd.h"
#include "gameconfiguration.h"

void InGameMenuPage::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   const auto w = GameConfiguration::getInstance()._view_width;
   const auto h = GameConfiguration::getInstance()._view_height;

   sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
   window.setView(view);

   for (auto& layer : _layer_stack)
   {
      if (layer->_visible)
      {
         layer->draw(window, states);
      }
   }
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

   for (const auto& layer : psd.getLayers())
   {
      // skip groups
      if (layer.getSectionDivider() != PSD::Layer::SectionDivider::None)
      {
         continue;
      }

      //      std::cout << "add layer: " << layer.getName() << ": " << layer.getLeft() << ", " << layer.getTop() << " (" << layer.getWidth()
      //                << " x " << layer.getHeight() << ")" << std::endl;

      // make all layers visible per default, don't trust the PSD :)
      auto tmp = std::make_shared<Layer>();
      tmp->_visible = true;
      tmp->_name = layer.getName();

      auto texture = std::make_shared<sf::Texture>();
      auto sprite = std::make_shared<sf::Sprite>();

      texture->create(static_cast<uint32_t>(layer.getWidth()), static_cast<uint32_t>(layer.getHeight()));
      texture->update(reinterpret_cast<const sf::Uint8*>(layer.getImage().getData().data()));

      sprite->setTexture(*texture, true);
      sprite->setPosition(static_cast<float>(layer.getLeft()), static_cast<float>(layer.getTop()));
      sprite->setColor(sf::Color{255, 255, 255, static_cast<uint8_t>(layer.getOpacity())});

      tmp->_texture = texture;
      tmp->_sprite = sprite;

      _layers[layer.getName()] = tmp;
      _layer_stack.push_back(tmp);
   }
}
