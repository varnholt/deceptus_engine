#include "testmechanism.h"

#include "../../src/framework/image/psd.h"

#include <iostream>

TestMechanism::TestMechanism()
{
   _rectangle_.setSize({200.f, 100.f});
   _rectangle_.setFillColor(sf::Color::Red);
   _rectangle_.setPosition({540.f, 310.f});

   _filename = "data/portal-test.psd";

   load();
}

void TestMechanism::load()
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
         std::cerr << "failed to create texture: " << layer.getName();
      }
   }
}

void TestMechanism::draw(sf::RenderTarget& target, sf::RenderTarget&)
{
   // debug rect
   // target.draw(_rectangle_);

   sf::RenderStates states;

   for (auto& layer : _layer_stack)
   {
      if (layer->_visible)
      {
         layer->draw(target, states);
      }
   }
}

void TestMechanism::update(const sf::Time&)
{
   // No-op for now
}
