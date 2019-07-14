#include "controlleroverlay.h"

#include "gameconfiguration.h"
#include "image/psd.h"

#include <iostream>


ControllerOverlay::ControllerOverlay()
{
   // load ingame psd
   PSD psd;
   psd.setColorFormat(PSD::ColorFormat::ABGR);
   psd.load("data/game/controller.psd");

   for (const auto& layer : psd.getLayers())
   {
      // std::cout << layer.getName() << std::endl;

      auto tmp = std::make_shared<Layer>();
      tmp->mVisible = layer.isVisible();

      auto texture = std::make_shared<sf::Texture>();
      auto sprite = std::make_shared<sf::Sprite>();

      texture->create(static_cast<uint32_t>(layer.getWidth()), static_cast<uint32_t>(layer.getHeight()));
      texture->update(reinterpret_cast<const sf::Uint8*>(layer.getImage().getData().data()));

      sprite->setTexture(*texture, true);
      sprite->setPosition(static_cast<float>(layer.getLeft()), static_cast<float>(layer.getTop()));
      sprite->setColor(sf::Color{255, 255, 255, static_cast<uint8_t>(layer.getOpacity())});

      tmp->mTexture = texture;
      tmp->mSprite = sprite;

      mLayers[layer.getName()] = tmp;
   }
}


/*
   black
   controller_bg
   analog_l
   analog_r
   button_a
   button_x
   button_b
   button_y
   dp_down
   dp_up
   dp_left
   dp_right
   lb
   rt
   rb
   lt
   view
   menu
   xbox
*/


void ControllerOverlay::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   auto w = GameConfiguration::getInstance().mViewWidth;
   auto h = GameConfiguration::getInstance().mViewHeight;

   // draw layers
   auto windowView = sf::View(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
   windowView.move(-5, -210);
   window.setView(windowView);

   auto controller_bg = mLayers["controller_bg"];
   auto analog_l      = mLayers["analog_l"];
   auto analog_r      = mLayers["analog_r"];
   auto button_a      = mLayers["button_a"];
   auto button_x      = mLayers["button_x"];
   auto button_b      = mLayers["button_b"];
   auto button_y      = mLayers["button_y"];
   auto dp_down       = mLayers["dp_down"];
   auto dp_up         = mLayers["dp_up"];
   auto dp_left       = mLayers["dp_left"];
   auto dp_right      = mLayers["dp_right"];
   auto lb            = mLayers["lb"];
   auto rt            = mLayers["rt"];
   auto rb            = mLayers["rb"];
   auto lt            = mLayers["lt"];
   auto view          = mLayers["view"];
   auto menu          = mLayers["menu"];
   auto xbox          = mLayers["xbox"];

   controller_bg->draw(window, states);
   analog_l->draw(window, states);
   analog_r->draw(window, states);
   button_a->draw(window, states);
   button_x->draw(window, states);
   button_b->draw(window, states);
   button_y->draw(window, states);
   dp_down->draw(window, states);
   dp_up->draw(window, states);
   dp_left->draw(window, states);
   dp_right->draw(window, states);
   lb->draw(window, states);
   rt->draw(window, states);
   rb->draw(window, states);
   lt->draw(window, states);
   view->draw(window, states);
   menu->draw(window, states);
   xbox->draw(window, states);
}
