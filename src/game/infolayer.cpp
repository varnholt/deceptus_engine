#include "infolayer.h"

#include "extratable.h"
#include "gameconfiguration.h"
#include "player.h"

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
}


void InfoLayer::draw(sf::RenderTarget& window)
{
   auto w = GameConfiguration::getInstance().mViewWidth;
   auto h = GameConfiguration::getInstance().mViewHeight;

   sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
   window.setView(view);

   int health = Player::getPlayer(0)->mExtraTable->mHealth->mHealth;
   int hearts = health / 20;

   for (auto i = 0; i < hearts; i++)
   {
      mHeartSprite.setPosition(
         static_cast<float>(5 * mFont.mCharWidth + i * 16),
         static_cast<float>(2)
      );

      window.draw(mHeartSprite);
   }

   std::vector<std::shared_ptr<sf::IntRect>> coords = mFont.getCoords(std::to_string(health));
   mFont.draw(window, coords, 5, 5);
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


