#include "levelmap.h"

#include "camerapane.h"
#include "console.h"
#include "door.h"
#include "extratable.h"
#include "globalclock.h"
#include "gameconfiguration.h"
#include "player.h"
#include "portal.h"

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


void LevelMap::loadLevelTexture(const std::filesystem::path& path)
{
   mLevelTexture.loadFromFile(path.string());
   mLevelSprite.setTexture(mLevelTexture);
}


void LevelMap::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   auto w = GameConfiguration::getInstance().mViewWidth;
   auto h = GameConfiguration::getInstance().mViewHeight;

   sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
   window.setView(view);

   auto layerLayout = mLayers["layout"];
   auto layerHideBorders = mLayers["hide_borders"];
   auto layerGrid = mLayers["grid"];
   auto layerBlue = mLayers["blue"];
   auto layerTextZoom = mLayers["text_zoom"];
   auto layerTextPan = mLayers["text_pan"];

   // 1 pixel in the level layer should be 8 pixels in the player world
   auto origin = Player::getPlayer(0)->getPixelPosition() * 0.125f;
   origin.x -= w/2;
   origin.y -= h/2;
   origin += CameraPane::getInstance().getLookVector();

   mLevelSprite.setOrigin(origin);

   layerBlue->draw(window, states);
   mLevelSprite.setColor(sf::Color{70, 70, 140, 255});
   window.draw(mLevelSprite, sf::BlendMode{sf::BlendAdd});
   layerGrid->draw(window, sf::BlendMode{sf::BlendAdd});
   layerHideBorders->draw(window, states);

   if (mZoomEnabled)
   {
      layerTextZoom->draw(window, states);
   }

   if (mPanEnabled)
   {
      layerTextPan->draw(window, states);
   }

   layerLayout->draw(window, states);

   // draw doors
   float scale = 1.0f;
   for (auto door : mDoors)
   {
      sf::VertexArray quad(sf::Quads, 4);
      quad[0].color = sf::Color::White;
      quad[1].color = sf::Color::White;
      quad[2].color = sf::Color::White;
      quad[3].color = sf::Color::White;

      auto pos = sf::Vector2f(door->getTilePosition().x * 3, door->getTilePosition().y * 3) + CameraPane::getInstance().getLookVector();

      quad[0].position = sf::Vector2f(static_cast<float>(pos.x * scale),         static_cast<float>(pos.y * scale));
      quad[1].position = sf::Vector2f(static_cast<float>(pos.x * scale + scale), static_cast<float>(pos.y * scale));
      quad[2].position = sf::Vector2f(static_cast<float>(pos.x * scale + scale), static_cast<float>(pos.y * scale + scale));
      quad[3].position = sf::Vector2f(static_cast<float>(pos.x * scale),         static_cast<float>(pos.y * scale + scale));

      window.draw(&quad[0], 4, sf::Quads);
   }

   // std::stringstream stream;
   // auto pos = Player::getPlayer(0)->getPixelPosition();
   // stream << "player pos: " << static_cast<int>(pos.x / TILE_WIDTH) << ", " << static_cast<int>(pos.y / TILE_HEIGHT);
   //
   // mFont.draw(window, mFont.getCoords(stream.str()), 5, 50);
   // mFont.draw(window, mFont.getCoords(Console::getInstance().getCommand()), 5, 100);
}


void LevelMap::setDoors(const std::vector<Door*>& doors)
{
   mDoors = doors;
}


void LevelMap::setPortals(const std::vector<Portal*>& portals)
{
   mPortals = portals;
}


