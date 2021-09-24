#include "infolayer.h"

#include "animationframedata.h"
#include "camerapane.h"
#include "console.h"
#include "extratable.h"
#include "gameconfiguration.h"
#include "framework/image/psd.h"
#include "framework/tools/globalclock.h"
#include "player/player.h"
#include "player/playerinfo.h"
#include "savestate.h"
#include "texturepool.h"

#include <iostream>
#include <sstream>


InfoLayer::InfoLayer()
{
   _font.load(
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
      tmp->_visible = layer.isVisible();

      auto texture = std::make_shared<sf::Texture>();
      auto sprite = std::make_shared<sf::Sprite>();

      texture->create(layer.getWidth(), layer.getHeight());
      texture->update(reinterpret_cast<const sf::Uint8*>(layer.getImage().getData().data()));

      sprite->setTexture(*texture, true);
      sprite->setPosition(static_cast<float>(layer.getLeft()), static_cast<float>(layer.getTop()));

      tmp->_texture = texture;
      tmp->_sprite = sprite;

      _layer_stack.push_back(tmp);
      _layers[layer.getName()] = tmp;
   }

   // load heart animation
   const auto t = sf::milliseconds(100);
   std::vector<sf::Time> ts;
   static constexpr auto frame_count = 6 * 8 + 7;
   for (auto i = 0; i < frame_count; i++)
   {
      ts.push_back(t);
   }

   AnimationFrameData frames {
      TexturePool::getInstance().get("data/sprites/health.png"),
      {0, 0},
      24, 24,
      frame_count,
      8,
      ts,
      0
   };

   _heart_animation._frames = frames._frames;
   _heart_animation._color_texture = frames._texture;
   _heart_animation.setFrameTimes(frames._frame_times);
   _heart_animation.setOrigin(frames._origin);
   _heart_animation._reset_to_first_frame = false;
}


void InfoLayer::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   const auto now = GlobalClock::getInstance().getElapsedTime();

   auto w = GameConfiguration::getInstance()._view_width;
   auto h = GameConfiguration::getInstance()._view_height;

   sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
   window.setView(view);

   auto layer_health = _layers["health"];
   auto layer_health_energy = _layers["health_energy"];
   auto layer_health_weapon = _layers["health_weapon"];

   if (layer_health_energy->_visible)
   {
       const auto health = (SaveState::getPlayerInfo().mExtraTable._health._health) * 0.01f;

       const auto healthLayerWidth  = layer_health_energy->_sprite->getTexture()->getSize().x * health;
       const auto healthLayerHeight = layer_health_energy->_sprite->getTexture()->getSize().y;

       layer_health_energy->_sprite->setTextureRect(
          sf::IntRect{
             0,
             0,
             static_cast<int32_t>(healthLayerWidth),
             static_cast<int32_t>(healthLayerHeight)
          }
       );

       // std::cout << "energy: " << healthLayerWidth << std::endl;

       auto t = (now - _show_time).asSeconds();
       const auto duration = 1.0f;
       t = (0.5f * (1.0f + cos((std::min(t, duration) / duration) * static_cast<float>(M_PI)))) * 200;

       layer_health->_sprite->setOrigin(t, 0.0f);
       layer_health_energy->_sprite->setOrigin(t, 0.0f);
       layer_health_weapon->_sprite->setOrigin(t, 0.0f);

       layer_health->draw(window, states);
       layer_health_energy->draw(window, states);
       layer_health_weapon->draw(window, states);
   }

   auto autosave = _layers["autosave"];
   if (autosave->_visible)
   {
      auto alpha = 0.5f * (1.0f + sin(now.asSeconds() * 2.0f));
      autosave->_sprite->setColor(sf::Color(255, 255, 255, static_cast<uint8_t>(alpha * 255)));
      autosave->draw(window, states);
   }

   // support cpan
   if (CameraPane::getInstance().isLookActive())
   {
       auto layer_cpan_up = _layers["cpan_up"];
       auto layer_cpan_down = _layers["cpan_down"];
       auto layer_cpan_left = _layers["cpan_left"];
       auto layer_cpan_right = _layers["cpan_right"];

       layer_cpan_up->draw(window, states);
       layer_cpan_down->draw(window, states);
       layer_cpan_left->draw(window, states);
       layer_cpan_right->draw(window, states);
   }
}


void InfoLayer::drawDebugInfo(sf::RenderTarget& window)
{
   auto w = GameConfiguration::getInstance()._view_width;
   auto h = GameConfiguration::getInstance()._view_height;

   sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
   window.setView(view);

   std::stringstream stream;
   auto pos = Player::getCurrent()->getPixelPositionf();
   stream << "player pos: " << static_cast<int>(pos.x / PIXELS_PER_TILE) << ", " << static_cast<int>(pos.y / PIXELS_PER_TILE);

   _font.draw(window, _font.getCoords(stream.str()), 510, 5);
}


void InfoLayer::drawConsole(sf::RenderTarget& window, sf::RenderStates states)
{
   auto w_view = GameConfiguration::getInstance()._view_width;
   auto h_view = GameConfiguration::getInstance()._view_height;

   sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w_view), static_cast<float>(h_view)));
   window.setView(view);

   auto layer_health = _layers["console"];
   layer_health->draw(window, states);

   auto w_screen = GameConfiguration::getInstance()._video_mode_width;
   auto h_screen = GameConfiguration::getInstance()._video_mode_height;

   sf::View view_screen(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w_screen), static_cast<float>(h_screen)));
   window.setView(view_screen);

   auto& console = Console::getInstance();
   const auto& command = console.getCommand();
   const auto& commands = console.getLog();

   static const auto offset_x = 16;
   static const auto offset_y = h_screen - 48;

   auto y = 0;
   for (auto it = commands.crbegin(); it != commands.crend(); ++it)
   {
      _font.draw(window, _font.getCoords(*it), offset_x, offset_y - ( (y + 1) * 14));
      y++;
   }

   auto bitmap_font = _font.getCoords(command);
   _font.draw(window, bitmap_font, offset_x, h_screen - 28);

   // draw cursor
   auto elapsed = GlobalClock::getInstance().getElapsedTime();
   if (static_cast<int32_t>(elapsed.asSeconds()) % 2 == 0)
   {
      _font.draw(window, _font.getCoords("_"), _font._text_width + offset_x, h_screen - 28);
   }
}


void InfoLayer::setLoading(bool loading)
{
   _layers["autosave"]->_visible = loading;

   _layers["health"]->_visible = !loading;
   _layers["health_energy"]->_visible = !loading;
   _layers["health_weapon"]->_visible = !loading;

   if (!loading && loading != _loading)
   {
       _show_time = GlobalClock::getInstance().getElapsedTime();
   }

   _loading = loading;
}


void InfoLayer::update(const sf::Time& dt)
{
   if (_heart_animation._paused)
   {
      return;
   }

   _heart_animation.update(dt);
}


void InfoLayer::playHeartAnimation()
{
   static const auto x = 100;
   static const auto y = 100;

   _heart_animation.setPosition(x, y);
   _heart_animation.updateVertices();
   _heart_animation.play();
}


void InfoLayer::drawHeartAnimation(sf::RenderTarget& window)
{
   _heart_animation.draw(window);
}


