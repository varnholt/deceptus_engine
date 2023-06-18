#include "infolayer.h"

#include "framework/image/psd.h"
#include "framework/tools/globalclock.h"
#include "framework/tools/log.h"
#include "game/animationframedata.h"
#include "game/camerapanorama.h"
#include "game/console.h"
#include "game/extratable.h"
#include "game/gameconfiguration.h"
#include "game/player/player.h"
#include "game/player/playerinfo.h"
#include "game/savestate.h"
#include "game/texturepool.h"

#ifdef __GNUC__
#define FMT_HEADER_ONLY
#include <fmt/core.h>
#else
#include <format>
namespace fmt = std;
#endif

#include <iostream>
#include <sstream>

namespace
{

using HighResDuration = std::chrono::high_resolution_clock::duration;
HighResDuration getRandomDuration(const HighResDuration& min_duration, const HighResDuration& max_duration)
{
   const auto min_duration_count = min_duration.count();
   const auto max_duration_count = max_duration.count();
   const auto random_duration_count = min_duration_count + (std::rand() % (max_duration_count - min_duration_count + 1));
   HighResDuration random_duration(random_duration_count);
   return random_duration;
}

static constexpr auto heart_layer_count = 10;
static constexpr auto heart_quarter_layer_count = heart_layer_count * 4;
}  // namespace

/*

Background

console

zone_the_sewers
zone_graveyard

item_slot_1

health area
   [x] character_window
   [x] item_sword_ammo
   [x] weapon_sword_icon
   [ ] weapon_none_icon
   [ ] weapon_sword_disabled_icon

40
...
1

health
   keep disabled until given
      [ ] hp_slot_13
      [ ] hp_slot_12
      ...
      [ ] hp_slot_02
      [ ] hp_slot_01

stamina bars
   energy_6
   energy_5
   energy_4
   energy_3
   energy_2
   energy_1

no_pause
skip_0
skip_1
skip_2
skip_3
autosave
cpan_right
cpan_down
cpan_left
cpan_up
cpan_up_enabled
cpan_down_enabled
cpan_left_enabled
cpan_right_enabled

weapon_slot_Y
weapon_slot_X
weapon_slot_B
weapon_slot_A
item_slot2_Y
item_slot2_X
item_slot2_B
item_slot2_A
*/

InfoLayer::InfoLayer()
{
   _font.load("data/game/font.png", "data/game/font.map");

   // load ingame psd
   PSD psd;
   psd.setColorFormat(PSD::ColorFormat::ABGR);
   psd.load("data/game/ingame_ui.psd");

   // Log::Info() << mFilename;

   for (const auto& layer : psd.getLayers())
   {
      // skip groups
      if (layer.getSectionDivider() != PSD::Layer::SectionDivider::None)
      {
         continue;
      }

      // Log::Info() << layer.getName();

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

   // init heart layers
   for (auto i = 1u; i <= heart_quarter_layer_count; i++)
   {
      _heart_layers.push_back(_layers[fmt::format("{}", i)]);
   }

   for (auto i = 1u; i <= 6; i++)
   {
      _stamina_layers.push_back(_layers[fmt::format("energy_{}", i)]);
   }

   _character_window_layer = _layers["character_window"];
   _item_sword_ammo_layer = _layers["item_sword_ammo"];
   _weapon_sword_icon_layer = _layers["weapon_sword_icon"];

   _slot_1_item_layer = _layers["item_slot_1"];
   _slot_2_item_layer = _layers["item_slot_2"];
   _slot_1_weapon_layer = _layers["weapon_slot_1"];
   _slot_2_weapon_layer = _layers["weapon_slot_2"];

   // load heart animation
   const auto t = sf::milliseconds(100);
   std::vector<sf::Time> ts;
   static constexpr auto frame_count = 6 * 8 + 7;
   for (auto i = 0; i < frame_count; i++)
   {
      ts.push_back(t);
   }

   AnimationFrameData frames{TexturePool::getInstance().get("data/sprites/health.png"), {0, 0}, 24, 24, frame_count, 8, ts, 0};
   _heart_animation._frames = frames._frames;
   _heart_animation._color_texture = frames._texture;
   _heart_animation.setFrameTimes(frames._frame_times);
   _heart_animation.setOrigin(frames._origin);
   _heart_animation._reset_to_first_frame = false;

   // init min/max durations for randomized animations
   using namespace std::chrono_literals;
   _animation_heart_duration_range = {8s, 12s};
   _animation_stamina_duration_range = {8s, 12s};
   _animation_skull_blink_duration_range = {8s, 12s};

   _animation_heart = _animation_pool.create("heart", 10.0f, 10.0f, false, true);
   _animation_stamina = _animation_pool.create("stamina", 20.0f, 20.0f, false, true);
   _animation_skull_blink = _animation_pool.create("skull_blink", 30.0f, 30.0f, false, true);
   _animation_hp_unlock_left = _animation_pool.create("hp_unlock_left", 0.0f, 0.0f, false, true);
   _animation_hp_unlock_right = _animation_pool.create("hp_unlock_right", 0.0f, 0.0f, false, true);

   _animation_heart->_reset_to_first_frame = false;
   _animation_stamina->_reset_to_first_frame = false;
   _animation_skull_blink->_reset_to_first_frame = false;
   _animation_hp_unlock_left->_reset_to_first_frame = false;
   _animation_hp_unlock_right->_reset_to_first_frame = false;
}

void InfoLayer::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   const auto now = GlobalClock::getInstance().getElapsedTime();

   const auto w = GameConfiguration::getInstance()._view_width;
   const auto h = GameConfiguration::getInstance()._view_height;

   sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
   window.setView(view);

   auto autosave = _layers["autosave"];
   if (autosave->_visible)
   {
      auto alpha = 0.5f * (1.0f + sin(now.asSeconds() * 2.0f));
      autosave->_sprite->setColor(sf::Color(255, 255, 255, static_cast<uint8_t>(alpha * 255)));
      autosave->draw(window, states);
   }

   // support cpan
   if (CameraPanorama::getInstance().isLookActive())
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

   if (!_loading)
   {
      const auto& extra_table = SaveState::getPlayerInfo()._extra_table;
      const auto heart_quarters = extra_table._health._health;

      _character_window_layer->draw(window, states);

      const auto stamina = static_cast<int32_t>(extra_table._health._stamina * 6.0f);
      for (auto i = 0; i < stamina; i++)
      {
         _stamina_layers[i]->draw(window, states);
      }

      for (auto i = 0; i < heart_quarters; i++)
      {
         _heart_layers[i]->draw(window, states);
      }

      _item_sword_ammo_layer->draw(window, states);
      _weapon_sword_icon_layer->draw(window, states);
      _slot_1_item_layer->draw(window, states);

      // _animation_pool.drawAnimations(window);
   }
}

void InfoLayer::drawDebugInfo(sf::RenderTarget& window)
{
   auto w = GameConfiguration::getInstance()._view_width;
   auto h = GameConfiguration::getInstance()._view_height;

   sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
   window.setView(view);

   std::stringstream stream_tl;
   std::stringstream stream_px;
   auto pos = Player::getCurrent()->getPixelPositionFloat();

   stream_tl << "player tl: " << static_cast<int>(pos.x / PIXELS_PER_TILE) << ", " << static_cast<int>(pos.y / PIXELS_PER_TILE);
   stream_px << "player px: " << static_cast<int>(pos.x) << ", " << static_cast<int>(pos.y);

   _font.draw(window, _font.getCoords(stream_tl.str()), 500, 5);
   _font.draw(window, _font.getCoords(stream_px.str()), 500, 20);
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
      _font.draw(window, _font.getCoords(*it), offset_x, offset_y - ((y + 1) * 14));
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

   if (!loading && loading != _loading)
   {
      _show_time = GlobalClock::getInstance().getElapsedTime();
   }

   _loading = loading;
}

void InfoLayer::update(const sf::Time& dt)
{
   updateAnimations(dt);

   if (_heart_animation._paused)
   {
      return;
   }

   _heart_animation.update(dt);
}

void InfoLayer::updateAnimations(const sf::Time& dt)
{
   if (!_next_animation_duration_heart.has_value())
   {
      _next_animation_duration_heart = getRandomDuration(_animation_heart_duration_range[0], _animation_heart_duration_range[1]);
      _animation_heart->pause();
   }

   if (!_next_animation_duration_stamina.has_value())
   {
      _next_animation_duration_stamina = getRandomDuration(_animation_stamina_duration_range[0], _animation_stamina_duration_range[1]);
      _animation_stamina->pause();
   }

   if (!_next_animation_duration_skull_blink.has_value())
   {
      _next_animation_duration_skull_blink =
         getRandomDuration(_animation_skull_blink_duration_range[0], _animation_skull_blink_duration_range[1]);
      _animation_skull_blink->pause();
   }

   const auto elapsed_micros = dt.asMicroseconds();
   HighResDuration elapsed_duration = std::chrono::microseconds(elapsed_micros);

   animation_duration_heart += elapsed_duration;
   animation_duration_stamina += elapsed_duration;
   animation_duration_skull_blink += elapsed_duration;

   if (animation_duration_heart > _next_animation_duration_heart)
   {
      animation_duration_heart = {};
      _next_animation_duration_heart.reset();

      _animation_heart->seekToStart();
      _animation_heart->play();
   }

   if (animation_duration_stamina > _next_animation_duration_stamina)
   {
      animation_duration_stamina = {};
      _next_animation_duration_stamina.reset();

      _animation_stamina->seekToStart();
      _animation_stamina->play();
   }

   if (animation_duration_skull_blink > _next_animation_duration_skull_blink)
   {
      animation_duration_skull_blink = {};
      _next_animation_duration_skull_blink.reset();

      _animation_skull_blink->seekToStart();
      _animation_skull_blink->play();
   }

   _animation_heart->update(dt);
   _animation_stamina->update(dt);
   _animation_skull_blink->update(dt);
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

// auto layer_health = _layers["health"];
// auto layer_health_energy = _layers["health_energy"];
// auto layer_health_weapon = _layers["health_weapon"];
//
// if (layer_health_energy->_visible)
// {
//     const auto health = (SaveState::getPlayerInfo().mExtraTable._health._health) * 0.01f;
//
//     const auto healthLayerWidth  = layer_health_energy->_sprite->getTexture()->getSize().x * health;
//     const auto healthLayerHeight = layer_health_energy->_sprite->getTexture()->getSize().y;
//
//     layer_health_energy->_sprite->setTextureRect(
//        sf::IntRect{
//           0,
//           0,
//           static_cast<int32_t>(healthLayerWidth),
//           static_cast<int32_t>(healthLayerHeight)
//        }
//     );
//
//     // Log::Info() << "energy: " << healthLayerWidth;
//
//     auto t = (now - _show_time).asSeconds();
//     const auto duration = 1.0f;
//     t = (0.5f * (1.0f + cos((std::min(t, duration) / duration) * static_cast<float>(M_PI)))) * 200;
//
//     layer_health->_sprite->setOrigin(t, 0.0f);
//     layer_health_energy->_sprite->setOrigin(t, 0.0f);
//     layer_health_weapon->_sprite->setOrigin(t, 0.0f);
//
//     layer_health->draw(window, states);
//     layer_health_energy->draw(window, states);
//     layer_health_weapon->draw(window, states);
// }
