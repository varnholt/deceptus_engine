#include "levelmap.h"

#include "camerapane.h"
#include "console.h"
#include "extratable.h"
#include "globalclock.h"
#include "gameconfiguration.h"
#include "mechanisms/door.h"
#include "mechanisms/portal.h"
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


void LevelMap::loadLevelTextures(
   const std::filesystem::path& grid,
   const std::filesystem::path& outlines
)
{
   mLevelGridTexture.loadFromFile(grid.string());
   mLevelGridSprite.setTexture(mLevelGridTexture);

   mLevelOutlineTexture.loadFromFile(outlines.string());
   mLevelOutlineSprite.setTexture(mLevelOutlineTexture);

   // that render texture should have the same size as our level textures
   mLevelRenderTexture.create(mLevelGridTexture.getSize().x, mLevelGridTexture.getSize().y);
}


void LevelMap::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   auto w = GameConfiguration::getInstance().mViewWidth;
   auto h = GameConfiguration::getInstance().mViewHeight;

   sf::Vector2f center;
   center += Player::getCurrent()->getPixelPositionf() * 0.125f;
   center += CameraPane::getInstance().getLookVector();
   center.x += mLevelGridSprite.getTexture()->getSize().x / 2.0f;
   center.y += mLevelGridSprite.getTexture()->getSize().y / 2.0f;
   center.x -= 220.0f;
   center.y -= 80.0f;

   sf::View levelView;
   levelView.setSize(static_cast<float>(mLevelGridSprite.getTexture()->getSize().x), static_cast<float>(mLevelGridSprite.getTexture()->getSize().y));
   levelView.setCenter(center);
   levelView.zoom(mZoom); // 1.5f works well, too
   mLevelGridSprite.setColor(sf::Color{70, 70, 140, 255});
   mLevelOutlineSprite.setColor(sf::Color{255, 255, 255, 80});
   mLevelRenderTexture.clear();
   mLevelRenderTexture.draw(mLevelGridSprite, sf::BlendMode{sf::BlendAdd});
   mLevelRenderTexture.draw(mLevelOutlineSprite, sf::BlendMode{sf::BlendAdd});
   drawLevelItems(mLevelRenderTexture);
   mLevelRenderTexture.setView(levelView);
   mLevelRenderTexture.display();

   // std::cout << "dx/dy: " << CameraPane::getInstance().getLookVector().x << " " << CameraPane::getInstance().getLookVector().y << std::endl;

   auto levelTextureSprite = sf::Sprite(mLevelRenderTexture.getTexture());
   levelTextureSprite.move(10.0f, 48.0f);

   // draw layers
   sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
   window.setView(view);

   auto layerLayout = mLayers["layout"];
   auto layerHideBorders = mLayers["hide_borders"];
   auto layerGrid = mLayers["grid"];
   auto layerBlue = mLayers["blue"];
   auto layerTextZoom = mLayers["text_zoom"];
   auto layerTextPan = mLayers["text_pan"];

   layerBlue->draw(window, states);

   window.draw(levelTextureSprite, sf::BlendMode{sf::BlendAdd});

   layerHideBorders->draw(window, states);

   if (mZoomEnabled)
   {
      layerTextZoom->draw(window, states);
   }

   if (CameraPane::getInstance().isLookActive())
   {
      layerTextPan->draw(window, states);
   }

   layerLayout->draw(window, states);


   // std::stringstream stream;
   // auto pos = Player::getPlayer(0)->getPixelPosition();
   // stream << "player pos: " << static_cast<int>(pos.x / TILE_WIDTH) << ", " << static_cast<int>(pos.y / TILE_HEIGHT);
   //
   // mFont.draw(window, mFont.getCoords(stream.str()), 5, 50);
   // mFont.draw(window, mFont.getCoords(Console::getInstance().getCommand()), 5, 100);
}


void LevelMap::setDoors(const std::vector<std::shared_ptr<GameMechanism>>& doors)
{
   mDoors = doors;
}


void LevelMap::setPortals(const std::vector<std::shared_ptr<GameMechanism>>& portals)
{
   mPortals = portals;
}


void LevelMap::drawLevelItems(sf::RenderTarget& target, sf::RenderStates)
{
   float scale = 3.0f;

   // draw grid
   for (auto y = 0u; y < target.getSize().y; y += 16)
   {
      sf::Vertex a(sf::Vector2f(0.0f, static_cast<float>(y)));
      sf::Vertex b(sf::Vector2f(static_cast<float>(target.getSize().x), static_cast<float>(y)));
      a.color.a = 30;
      b.color.a = 30;

      sf::Vertex line[] = {a, b};

      target.draw(line, 2, sf::Lines);
   }

   for (auto x = 0u; x < target.getSize().x; x += 16)
   {
      sf::Vertex a(sf::Vector2f(static_cast<float>(x), 0.0f));
      sf::Vertex b(sf::Vector2f(static_cast<float>(x), static_cast<float>(target.getSize().y)));
      a.color.a = 30;
      b.color.a = 30;

      sf::Vertex line[] = {a, b};

      target.draw(line, 2, sf::Lines);
   }

   // draw doors
   float doorWidth = 2.0f;
   float doorHeight = 9.0f;

   for (auto d : mDoors)
   {
      auto door = std::dynamic_pointer_cast<Door>(d);

      sf::VertexArray quad(sf::Quads, 4);
      quad[0].color = sf::Color::White;
      quad[1].color = sf::Color::White;
      quad[2].color = sf::Color::White;
      quad[3].color = sf::Color::White;

      auto pos = sf::Vector2f(static_cast<float>(door->getTilePosition().x), static_cast<float>(door->getTilePosition().y));

      quad[0].position = sf::Vector2f(static_cast<float>(pos.x * scale),             static_cast<float>(pos.y * scale));
      quad[1].position = sf::Vector2f(static_cast<float>(pos.x * scale + doorWidth), static_cast<float>(pos.y * scale));
      quad[2].position = sf::Vector2f(static_cast<float>(pos.x * scale + doorWidth), static_cast<float>(pos.y * scale + doorHeight));
      quad[3].position = sf::Vector2f(static_cast<float>(pos.x * scale),             static_cast<float>(pos.y * scale + doorHeight));

      target.draw(&quad[0], 4, sf::Quads);
   }

   // draw portals
   float portalWidth = 3.0f;
   float portalHeight = 6.0f;
   for (auto p : mPortals)
   {
      sf::VertexArray quad(sf::Quads, 4);
      quad[0].color = sf::Color::Red;
      quad[1].color = sf::Color::Red;
      quad[2].color = sf::Color::Red;
      quad[3].color = sf::Color::Red;

      auto portal = std::dynamic_pointer_cast<Portal>(p);
      auto pos = sf::Vector2f(portal->getTilePosition().x, portal->getTilePosition().y);

      quad[0].position = sf::Vector2f(static_cast<float>(pos.x * scale),               static_cast<float>(pos.y * scale));
      quad[1].position = sf::Vector2f(static_cast<float>(pos.x * scale + portalWidth), static_cast<float>(pos.y * scale));
      quad[2].position = sf::Vector2f(static_cast<float>(pos.x * scale + portalWidth), static_cast<float>(pos.y * scale + portalHeight));
      quad[3].position = sf::Vector2f(static_cast<float>(pos.x * scale),               static_cast<float>(pos.y * scale + portalHeight));

      target.draw(&quad[0], 4, sf::Quads);
   }

   // draw player
   auto playerWidth = 5.0f;
   auto playerHeight = 4;
   sf::CircleShape square(playerWidth, static_cast<uint32_t>(playerHeight));
   square.setPosition(Player::getCurrent()->getPixelPositionf() * 0.125f);
   square.move(-playerWidth, -playerHeight * 2.0f);
   square.setFillColor(sf::Color::White);
   target.draw(square);
}



