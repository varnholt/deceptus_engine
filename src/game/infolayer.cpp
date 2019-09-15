#include "infolayer.h"

#include "camerapane.h"
#include "console.h"
#include "extratable.h"
#include "globalclock.h"
#include "gameconfiguration.h"
#include "player.h"
#include "playerinfo.h"

#include "image/psd.h"

#include <iostream>
#include <sstream>


InfoLayer::InfoLayer()
{
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
   const auto now = GlobalClock::getInstance()->getElapsedTime();

   auto w = GameConfiguration::getInstance().mViewWidth;
   auto h = GameConfiguration::getInstance().mViewHeight;

   sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
   window.setView(view);

   auto layerHealth = mLayers["health"];
   auto layerHealthEnergy = mLayers["health_energy"];
   auto layerHealthWeapon = mLayers["health_weapon"];

   if (layerHealthEnergy->mVisible)
   {
       const auto health = (PlayerInfo::getCurrent().mExtraTable.mHealth->mHealth) * 0.01f;

       const auto healthLayerWidth  = layerHealthEnergy->mSprite->getTexture()->getSize().x * health;
       const auto healthLayerHeight = layerHealthEnergy->mSprite->getTexture()->getSize().y;

       layerHealthEnergy->mSprite->setTextureRect(
          sf::IntRect{
             0,
             0,
             static_cast<int32_t>(healthLayerWidth),
             static_cast<int32_t>(healthLayerHeight)
          }
       );

       // std::cout << "energy: " << healthLayerWidth << std::endl;

       auto t = (now - mShowTime).asSeconds();
       const auto duration = 1.0f;
       t = (0.5f * (1.0f + cos((std::min(t, duration) / duration) * static_cast<float>(M_PI)))) * 200;

       layerHealth->mSprite->setOrigin(t, 0.0f);
       layerHealthEnergy->mSprite->setOrigin(t, 0.0f);
       layerHealthWeapon->mSprite->setOrigin(t, 0.0f);

       layerHealth->draw(window, states);
       layerHealthEnergy->draw(window, states);
       layerHealthWeapon->draw(window, states);
   }

   auto autosave = mLayers["autosave"];
   if (autosave->mVisible)
   {
      auto alpha = 0.5f * (1.0f + sin(now.asSeconds() * 2.0f));
      autosave->mSprite->setColor(sf::Color(255, 255, 255, static_cast<uint8_t>(alpha * 255)));
      autosave->draw(window, states);
   }

   // support cpan
   if (CameraPane::getInstance().isLookActive())
   {
       auto layerCameraPaneUp = mLayers["cpan_up"];
       auto layerCameraPaneDown = mLayers["cpan_down"];
       auto layerCameraPaneLeft = mLayers["cpan_left"];
       auto layerCameraPaneRight = mLayers["cpan_right"];

       layerCameraPaneUp->draw(window, states);
       layerCameraPaneDown->draw(window, states);
       layerCameraPaneLeft->draw(window, states);
       layerCameraPaneRight->draw(window, states);
   }
}


void InfoLayer::drawDebugInfo(sf::RenderTarget& window)
{
   auto w = GameConfiguration::getInstance().mViewWidth;
   auto h = GameConfiguration::getInstance().mViewHeight;

   sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
   window.setView(view);

   std::stringstream stream;
   auto pos = Player::getCurrent()->getPixelPosition();
   stream << "player pos: " << static_cast<int>(pos.x / PIXELS_PER_TILE) << ", " << static_cast<int>(pos.y / PIXELS_PER_TILE);

   mFont.draw(window, mFont.getCoords(stream.str()), 360, 5);
}


void InfoLayer::drawConsole(sf::RenderTarget& window)
{
   auto w = GameConfiguration::getInstance().mViewWidth;
   auto h = GameConfiguration::getInstance().mViewHeight;

   sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
   window.setView(view);

   auto& console = Console::getInstance();
   const auto& command = console.getCommand();
   const auto& commands = console.getLog();

   static const auto offset = 240;
   auto y = 0;
   for (auto it = commands.crbegin(); it != commands.crend(); ++it)
   {
      mFont.draw(window, mFont.getCoords(*it), 5, offset - ( (y + 1) * 14));
      y++;
   }

   auto bitmapFont = mFont.getCoords(command);
   mFont.draw(window, bitmapFont, 5, offset);

   // draw cursor
   auto elapsed = GlobalClock::getInstance()->getElapsedTime();
   if (static_cast<int32_t>(elapsed.asSeconds()) % 2 == 0)
   {
      mFont.draw(window, mFont.getCoords("_"), mFont.mTextWidth + 5, offset);
   }
}


void InfoLayer::setLoading(bool loading)
{
   mLayers["autosave"]->mVisible = loading;

   mLayers["health"]->mVisible = !loading;
   mLayers["health_energy"]->mVisible = !loading;
   mLayers["health_weapon"]->mVisible = !loading;

   if (!loading && loading != mLoading)
   {
       mShowTime = GlobalClock::getInstance()->getElapsedTime();
   }

   mLoading = loading;
}


