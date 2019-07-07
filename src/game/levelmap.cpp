#include "levelmap.h"

#include "console.h"
#include "extratable.h"
#include "globalclock.h"
#include "gameconfiguration.h"
#include "player.h"

#include "image/psd.h"

#include <iostream>
#include <sstream>


LevelMap::LevelMap()
{
   mFont.load(
      "data/game/font.png",
      "data/game/font.map"
   );

   // load ingame psd
   PSD psd;
   psd.setColorFormat(PSD::ColorFormat::ABGR);
   psd.load("data/game/map.psd");

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
      sprite->setColor(sf::Color{255, 255, 255, static_cast<uint8_t>(layer.getOpacity())});

      tmp->mTexture = texture;
      tmp->mSprite = sprite;

      mLayerStack.push_back(tmp);
      mLayers[layer.getName()] = tmp;
   }
}


void LevelMap::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   auto w = GameConfiguration::getInstance().mViewWidth;
   auto h = GameConfiguration::getInstance().mViewHeight;

   sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
   window.setView(view);

   // for (auto layer : mLayerStack)
   // {
   //    layer->draw(window, states);
   // }

   auto layerLayout = mLayers["layout"];
   auto layerHideBorders = mLayers["hide_borders"];
   auto layerGrid = mLayers["grid"];
   auto layerBlue = mLayers["blue"];
   auto layerTextZoom = mLayers["text_zoom"];
   auto layerTextPan = mLayers["text_pan"];

   layerBlue->draw(window, states);
   layerGrid->draw(window, states);
   layerHideBorders->draw(window, states);
   layerTextZoom->draw(window, states);
   layerTextPan->draw(window, states);
   layerLayout->draw(window, states);

   // location_center
   // location_crosshhair
   // map_keys
   // zoom_slider
   // zoom_slider_handle
   // zoom_scale

   // auto layerHealth = mLayers["health"];
   // auto layerHealthEnergy = mLayers["health_energy"];
   // auto layerHealthWeapon = mLayers["health_weapon"];
   //
   // if (layerHealthEnergy->mVisible)
   // {
   //     layerHealth->draw(window, states);
   //     layerHealthEnergy->draw(window, states);
   //     layerHealthWeapon->draw(window, states);
   // }
}


void LevelMap::drawMapInfo(sf::RenderTarget& window)
{
   // auto w = GameConfiguration::getInstance().mViewWidth;
   // auto h = GameConfiguration::getInstance().mViewHeight;
   //
   // sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
   // window.setView(view);
   //
   // std::stringstream stream;
   // auto pos = Player::getPlayer(0)->getPixelPosition();
   // stream << "player pos: " << static_cast<int>(pos.x / TILE_WIDTH) << ", " << static_cast<int>(pos.y / TILE_HEIGHT);
   //
   // mFont.draw(window, mFont.getCoords(stream.str()), 5, 50);
   // mFont.draw(window, mFont.getCoords(Console::getInstance().getCommand()), 5, 100);
}


