#include "forestscene.h"

#include "framework/image/psd.h"
#include "framework/tools/localization.h"
#include "framework/tools/log.h"
#include "game/config/gameconfiguration.h"

#include <math.h>
#include <iostream>
#include <stdexcept>

ForestScene::ForestScene()
{
   _text = std::make_unique<sf::Text>(*_font, sf::Text::Data{});
   _text->setCharacterSize(12);
   // mText.setString("Congratulations!\nYou completed the game!");
   _text->setString("Geschafft!\nAlles Gute zum Geburtstag, Malte!");
   _text->setFillColor(sf::Color{232, 219, 243});

   // load ingame psd
   PSD psd;
   psd.setColorFormat(PSD::ColorFormat::ABGR);
   psd.load("data/scenes/forest.psd");

   for (const auto& layer : psd.getLayers())
   {
      // skip groups
      if (!layer.isImageLayer())
      {
         continue;
      }

      auto tmp = std::make_shared<Layer>();
      tmp->_visible = true;  // layer.isVisible();

      try
      {
         auto texture_opt = sf::Texture::create(sf::Vector2u{static_cast<uint32_t>(layer.getWidth()), static_cast<uint32_t>(layer.getHeight())});
         if (!texture_opt.hasValue())
         {
            throw std::runtime_error("failed to create texture");
         }
         auto texture = std::make_shared<sf::Texture>(std::move(*texture_opt));

         auto sprite = std::make_shared<sf::Sprite>();
         texture->update(reinterpret_cast<const uint8_t*>(layer.getImage().getData().data()));

         sprite->position = {static_cast<float>(layer.getLeft()), static_cast<float>(layer.getTop())};
         sprite->color = sf::Color{255, 255, 255, static_cast<uint8_t>(layer.getOpacity())};

         tmp->_texture = texture;
         tmp->_sprite = sprite;

         _layers[layer.getName()] = tmp;
         _layer_stack.push_back(tmp);
      }
      catch (...)
      {
         Log::Fatal() << "failed to create texture: " << layer.getName();
      }
   }
}

void ForestScene::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   auto w = GameConfiguration::getInstance()._view_width;
   auto h = GameConfiguration::getInstance()._view_height;

   // draw layers
   const sf::View view = sf::View::fromRect(sf::FloatRect{{0.0f, 0.0f}, {static_cast<float>(w), static_cast<float>(h)}});
   states.view = view;

   for (auto& layer : _layer_stack)
   {
      layer->draw(window, states);
   }

   // draw text
   const auto rect = _text->getGlobalBounds();
   const auto left = w / 2 - rect.size.x / 2;
   _text->position = {floor(left), 82};
   window.draw(*_text, states);
}

/*
                                360px
                                |
   +--------------------------------------------------------+
   |                  +-------------------+                 |
   |                  | Congratulations   |                 |
   |                  | You won the game! |                 |
   |                  +-------------------+                 |
   +--------------------------------------------------------+
                      |                   |                 720px
                      |                   |
                      rect.size.x = 200px
*/

/*
   c6
   c5
   c4
   c3
   c2
   c1
   thunder
   mountains_l1
   mountains_l2
   static_fog_1
   mountains_l3
   static_fog_2
   mountains_l4
   mfog_1
   mfog_2
   mfog_3
   mountains_l5
   trees_1
   trees_2
   static_fog_3
*/

void ForestScene::update(const sf::Time& time)
{
   _layers["mfog_1"]->_sprite->position += sf::Vector2f{3.0f * time.asSeconds(), 0.0f};
   _layers["mfog_2"]->_sprite->position += sf::Vector2f{2.0f * time.asSeconds(), 0.0f};
   _layers["mfog_3"]->_sprite->position += sf::Vector2f{time.asSeconds(), 0.0f};
}
