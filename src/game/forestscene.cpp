#include "forestscene.h"

#include "gameconfiguration.h"
#include "image/psd.h"

#include <math.h>
#include <iostream>


ForestScene::ForestScene()
{
   if (mFont.loadFromFile("data/fonts/deceptum.ttf"))
   {
      mText.setScale(0.25f, 0.25f);
      mText.setFont(mFont);
      mText.setCharacterSize(48);
      mText.setString("Du hast es geschafft!\nAlles Gute zum Geburtstag, Lasse!\n- Dein Papa");
      mText.setFillColor(sf::Color{232, 219, 243});
   }
   else
   {
      std::cerr << "font load fuckup" << std::endl;
   }

   // load ingame psd
   PSD psd;
   psd.setColorFormat(PSD::ColorFormat::ABGR);
   psd.load("data/scenes/forest.psd");

   // std::cout << mFilename << std::endl;

   for (const auto& layer : psd.getLayers())
   {
      // skip groups
      if (layer.getSectionDivider() != PSD::Layer::SectionDivider::None)
      {
         continue;
      }

      // std::cout << layer.getName() << std::endl;

      auto tmp = std::make_shared<Layer>();
      tmp->mVisible = true; // layer.isVisible();


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
      mLayerStack.push_back(tmp);
   }
}


void ForestScene::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   auto w = GameConfiguration::getInstance().mViewWidth;
   auto h = GameConfiguration::getInstance().mViewHeight;

   // draw layers
   sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
   window.setView(view);

   for (auto& layer : mLayerStack)
   {
      layer->draw(window, states);
   }

   // draw text
   const auto rect = mText.getGlobalBounds();
   const auto left = 143;
   const auto x = left + (202 - rect.width) * 0.5f;
   mText.setPosition(floor(x), 82);
   window.draw(mText, states);
}


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
   mLayers["mfog_1"]->mSprite->move(3.0f * time.asSeconds(), 0.0f);
   mLayers["mfog_2"]->mSprite->move(2.0f * time.asSeconds(), 0.0f);
   mLayers["mfog_3"]->mSprite->move(time.asSeconds(), 0.0f);
}