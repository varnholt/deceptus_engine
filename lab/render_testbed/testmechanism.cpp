#include "testmechanism.h"

#include "../../src/framework/image/psd.h"

#include <iostream>

TestMechanism::TestMechanism()
{
   _rectangle_.setSize({200.f, 100.f});
   _rectangle_.setFillColor(sf::Color::Red);
   _rectangle_.setPosition({540.f, 310.f});

   _origin_shape.setRadius(1.0f);
   _origin_shape.setFillColor(sf::Color::Red);

   _filename = "data/portal-test.psd";

   load();
}

void TestMechanism::load()
{
   PSD psd;
   psd.setColorFormat(PSD::ColorFormat::ABGR);
   psd.load(_filename);

   int32_t pa_index = 0;
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

         const auto pos = sf::Vector2f{static_cast<float>(layer.getLeft()), static_cast<float>(layer.getTop())};
         sprite->setPosition(pos);
         sprite->setColor(sf::Color(255u, 255u, 255u, static_cast<uint8_t>(opacity)));

         tmp->_texture = texture;
         tmp->_sprite = sprite;
         tmp->_visible = layer.isVisible();

         _layer_stack.push_back(tmp);
         _layers[layer.getName()] = tmp;

         if (layer.getName().starts_with("pa_"))
         {
            std::cout << layer.getName() << std::endl;
            const auto origin = sf::Vector2f{texture->getSize().x * 0.5f, texture->getSize().y * 0.5f};
            tmp->_sprite->setOrigin(origin);
            sprite->setPosition(origin + pos);
            _pa[pa_index++] = tmp;
            _origin = origin + pos;
         }
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

   // draw all layers
   for (auto& layer : _layer_stack)
   {
      if (layer->_visible)
      {
         layer->draw(target, states);
      }
   }

   // draw pa
   std::ranges::for_each(
      _pa,
      [&target, states](const auto& pa)
      {
         //
         pa->draw(target, states);
      }
   );

   target.draw(_origin_shape);
}

void TestMechanism::update(const sf::Time& dt)
{
   // No-op for now
   _elapsed += dt.asSeconds();

   _origin_shape.setPosition(_origin);

   std::ranges::for_each(
      _pa,
      [this](const auto& pa)
      {
         //
         pa->_sprite->setRotation(sf::radians(_elapsed));
      }
   );
}
