#include "infolayer.h"

#include "extratable.h"
#include "globalclock.h"
#include "gameconfiguration.h"
#include "player.h"

#include "image/psd.h"

#include <iostream>
#include <sstream>


InfoLayer::InfoLayer()
{
   mHeartTexture.loadFromFile("data/game/info.png");
   mHeartSprite.setTexture(mHeartTexture);
   mHeartSprite.setTextureRect(
      sf::IntRect(
         0,
         0,
         16,
         16
      )
   );

   mFont.load(
      "data/game/font.png",
      "data/game/font.map"
   );

   // load ingame psd
   PSD psd;
   psd.setColorFormat(PSD::ColorFormat::ABGR);
   psd.load("data/game/ingame_ui.psd");

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
      tmp->mVisible = layer.isVisible();

      auto texture = std::make_shared<sf::Texture>();
      auto sprite = std::make_shared<sf::Sprite>();

      texture->create(layer.getWidth(), layer.getHeight());
      texture->update(reinterpret_cast<const sf::Uint8*>(layer.getImage().getData().data()));

      sprite->setTexture(*texture, true);
      sprite->setPosition(static_cast<float>(layer.getLeft()), static_cast<float>(layer.getTop()));

      tmp->mTexture = texture;
      tmp->mSprite = sprite;

      mLayerStack.push_back(tmp);
      mLayers[layer.getName()] = tmp;
   }
}


void InfoLayer::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   auto w = GameConfiguration::getInstance().mViewWidth;
   auto h = GameConfiguration::getInstance().mViewHeight;

   sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
   window.setView(view);

   auto health = (Player::getPlayer(0)->mExtraTable->mHealth->mHealth) * 0.01f;

   //
   // for (auto i = 0; i < hearts; i++)
   // {
   //    mHeartSprite.setPosition(
   //       static_cast<float>(5 * mFont.mCharWidth + i * 16),
   //       static_cast<float>(2)
   //    );
   //
   //    window.draw(mHeartSprite);
   // }
   //
   // std::vector<std::shared_ptr<sf::IntRect>> coords = mFont.getCoords(std::to_string(health));
   // mFont.draw(window, coords, 5, 5);

   // for (auto& layer : mLayerStack)
   // {
   //    if (layer->mVisible)
   //    {
   //       layer->draw(window, states);
   //    }
   // }

   auto energy = mLayers["health_energy"];

   energy->mSprite->setTextureRect(
      sf::IntRect{
         0,
         0,
         static_cast<int32_t>(energy->mSprite->getTexture()->getSize().x * health),
         static_cast<int32_t>(energy->mSprite->getTexture()->getSize().y)
      }
   );

   mLayers["health"]->draw(window, states);
   mLayers["health_energy"]->draw(window, states);
   mLayers["health_weapon"]->draw(window, states);

   auto autosave = mLayers["autosave"];
   if (autosave->mVisible)
   {
      auto alpha = 0.5f * (1.0f + sin(GlobalClock::getInstance()->getElapsedTime().asSeconds() * 2.0f));
      autosave->mSprite->setColor(sf::Color(255, 255, 255, alpha * 255));
      autosave->draw(window, states);
   }
}


void InfoLayer::drawDebugInfo(sf::RenderTarget& window)
{
   auto w = GameConfiguration::getInstance().mViewWidth;
   auto h = GameConfiguration::getInstance().mViewHeight;

   sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
   window.setView(view);

   std::stringstream stream;
   auto pos = Player::getPlayer(0)->getPixelPosition();
   stream << "player pos: " << static_cast<int>(pos.x / TILE_WIDTH) << ", " << static_cast<int>(pos.y / TILE_HEIGHT);

   std::vector<std::shared_ptr<sf::IntRect>> coords = mFont.getCoords(stream.str());
   mFont.draw(window, coords, 5, 50);
}


void InfoLayer::setLoading(bool loading)
{
   mLayers["autosave"]->mVisible = loading;
}


