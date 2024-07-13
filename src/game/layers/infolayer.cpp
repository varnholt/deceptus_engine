#include "infolayer.h"

#include "framework/image/psd.h"
#include "framework/tools/globalclock.h"
#include "framework/tools/log.h"
#include "game/animation/animationframedata.h"
#include "game/camera/camerapanorama.h"
#include "game/config/gameconfiguration.h"
#include "game/console.h"
#include "game/inventoryitemdescriptionreader.h"
#include "game/level/roomupdater.h"
#include "game/player/extratable.h"
#include "game/player/player.h"
#include "game/player/playerinfo.h"
#include "game/savestate.h"
#include "game/texturepool.h"

#if defined __GNUC__ && __linux__
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

   const auto factor = 0.01f * (std::rand() % 100);
   const auto delta = max_duration_count - min_duration_count;

   const auto random_duration_count = min_duration_count + static_cast<HighResDuration::rep>(factor * delta);
   HighResDuration random_duration(random_duration_count);
   return random_duration;
}

constexpr auto heart_layer_count = 10;
constexpr auto heart_quarter_layer_count = heart_layer_count * 4;

constexpr auto icon_width = 38;
constexpr auto icon_height = 38;

constexpr auto frame_0_pos_x_px = 5;
constexpr auto frame_0_pos_y_px = 16;
constexpr auto frame_1_pos_x_px = 43;
constexpr auto frame_1_pos_y_px = 16;

constexpr auto heart_pos_x_px = 81.0f;
constexpr auto heart_pos_y_px = 19.0f;
constexpr auto stamina_pos_x_px = 81.0f;
constexpr auto stamina_pos_y_px = 28.0f;
constexpr auto skull_pos_x_px = 32.0f;
constexpr auto skull_pos_y_px = 0.0f;
}  // namespace


InfoLayer::InfoLayer()
{
   _font.load("data/game/font.png", "data/game/font.map");

   // load ingame psd
   PSD psd;
   psd.setColorFormat(PSD::ColorFormat::ABGR);
   psd.load("data/game/ingame_ui.psd");

   for (const auto& layer : psd.getLayers())
   {
      // skip groups
      if (!layer.isImageLayer())
      {
         continue;
      }

      auto tmp = std::make_shared<Layer>();
      tmp->_visible = layer.isVisible();

      auto texture = std::make_shared<sf::Texture>();
      auto sprite = std::make_shared<sf::Sprite>();

      if (!texture->create(layer.getWidth(), layer.getHeight()))
      {
         Log::Fatal() << "failed to create texture: " << layer.getName();
      }

      texture->update(reinterpret_cast<const sf::Uint8*>(layer.getImage().getData().data()));

      sprite->setTexture(*texture, true);
      sprite->setPosition(static_cast<float>(layer.getLeft()), static_cast<float>(layer.getTop()));

      tmp->_texture = texture;
      tmp->_sprite = sprite;

      _layer_stack.push_back(tmp);
      _layers[layer.getName()] = tmp;
   }

   // init heart layers
   _heart_layers.reserve(heart_quarter_layer_count);
   for (auto i = 1u; i <= heart_quarter_layer_count; i++)
   {
      _heart_layers.push_back(_layers[fmt::format("{}", i)]);
   }

   _stamina_layers.reserve(6);
   for (auto i = 1u; i <= 6; i++)
   {
      _stamina_layers.push_back(_layers[fmt::format("energy_{}", i)]);
   }

   _character_window_layer = _layers["character_window"];

   _slot_item_layers[0] = _layers["item_slot_X"];
   _slot_item_layers[1] = _layers["item_slot_Y"];

   // load heart animation
   const auto t = sf::milliseconds(100);
   std::vector<sf::Time> ts;
   constexpr auto frame_count = 6 * 8 + 7;
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

   _animation_pool.setGarbageCollectorEnabled(false);  // don't clear animations

   _animation_heart = _animation_pool.create("heart", heart_pos_x_px, heart_pos_y_px, false, false);
   _animation_stamina = _animation_pool.create("stamina", stamina_pos_x_px, stamina_pos_y_px, false, false);
   _animation_skull_blink = _animation_pool.create("skull_blink", skull_pos_x_px, skull_pos_y_px, false, false);
   _animation_hp_unlock_left = _animation_pool.create("hp_unlock_left", 0.0f, 0.0f, false, false);
   _animation_hp_unlock_right = _animation_pool.create("hp_unlock_right", 0.0f, 0.0f, false, false);

   _animation_heart->_reset_to_first_frame = false;
   _animation_stamina->_reset_to_first_frame = false;
   _animation_skull_blink->_reset_to_first_frame = false;
   _animation_hp_unlock_left->_reset_to_first_frame = false;
   _animation_hp_unlock_right->_reset_to_first_frame = false;

   loadInventoryItems();
}

//---------------------------------------------------------------------------------------------------------------------
void InfoLayer::loadInventoryItems()
{
   const auto& inventory = SaveState::getPlayerInfo()._inventory;
   const auto& inventory_item_descriptions = inventory._descriptions;
   _inventory_texture = TexturePool::getInstance().get("data/sprites/inventory_items.png");

   std::ranges::for_each(
      inventory_item_descriptions,
      [this](const auto& image)
      {
         // store sprites
         sf::Sprite sprite;
         sprite.setTexture(*_inventory_texture);
         sprite.setTextureRect({image._x_px, image._y_px, icon_width, icon_height});
         _sprites[image._name] = sprite;
      }
   );

   _inventory_sprites[0].setTexture(*_inventory_texture);
   _inventory_sprites[0].setTextureRect({});
   _inventory_sprites[0].setPosition(frame_0_pos_x_px, frame_0_pos_y_px);

   _inventory_sprites[1].setTexture(*_inventory_texture);
   _inventory_sprites[1].setTextureRect({});
   _inventory_sprites[1].setPosition(frame_1_pos_x_px, frame_1_pos_y_px);
}

//---------------------------------------------------------------------------------------------------------------------
void InfoLayer::updateInventoryItems()
{
   const auto& inventory = SaveState::getPlayerInfo()._inventory;
   for (auto i = 0u; i < inventory._slots.size(); i++)
   {
      const auto slot = inventory._slots[i];

      if (slot.empty())
      {
         continue;
      }

      const auto& sprite = _sprites[slot];
      _inventory_sprites[i].setTextureRect(sprite.getTextureRect());
   }
}

//---------------------------------------------------------------------------------------------------------------------
void InfoLayer::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   const auto now = GlobalClock::getInstance().getElapsedTime();

   const auto w = GameConfiguration::getInstance()._view_width;
   const auto h = GameConfiguration::getInstance()._view_height;

   const sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
   window.setView(view);

   auto autosave = _layers["autosave"];
   if (autosave->_visible)
   {
      const auto alpha = 0.5f * (1.0f + sin(now.asSeconds() * 2.0f));
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

      drawInventoryItem(window, states);

      if (_animation_duration_heart < _next_animation_duration_heart)
      {
         _animation_heart->draw(window, states);
      }

      if (_animation_duration_stamina < _next_animation_duration_stamina)
      {
         _animation_stamina->draw(window, states);
      }

      if (_animation_duration_skull_blink < _next_animation_duration_skull_blink)
      {
         _animation_skull_blink->draw(window, states);
      }
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
   std::stringstream room_name;
   auto pos = Player::getCurrent()->getPixelPositionFloat();

   stream_tl << "player tl: " << static_cast<int>(pos.x / PIXELS_PER_TILE) << ", " << static_cast<int>(pos.y / PIXELS_PER_TILE);
   stream_px << "player px: " << static_cast<int>(pos.x) << ", " << static_cast<int>(pos.y);
   room_name << "room: " << RoomUpdater::getCurrentRoomName();

   _font.draw(window, _font.getCoords(stream_tl.str()), 500, 5);
   _font.draw(window, _font.getCoords(stream_px.str()), 500, 20);
   _font.draw(window, _font.getCoords(room_name.str()), 498, 35);
}

void InfoLayer::drawConsole(sf::RenderTarget& window, sf::RenderStates states)
{
   const auto w_view = GameConfiguration::getInstance()._view_width;
   const auto h_view = GameConfiguration::getInstance()._view_height;
   const auto w_screen = GameConfiguration::getInstance()._video_mode_width;
   const auto h_screen = GameConfiguration::getInstance()._video_mode_height;
   const auto& console = Console::getInstance();
   const auto& command = console.getCommand();
   const auto& commands = console.getLog();
   constexpr auto offset_x = 16;
   static const auto offset_y = h_screen - 48;

   sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w_view), static_cast<float>(h_view)));
   window.setView(view);

   const auto& layer_health = _layers["console"];
   layer_health->draw(window, states);

   sf::View view_screen(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w_screen), static_cast<float>(h_screen)));
   window.setView(view_screen);

   auto y = 0;
   for (auto it = commands.crbegin(); it != commands.crend(); ++it)
   {
      _font.draw(window, _font.getCoords(*it), offset_x, offset_y - ((y + 1) * 14));
      y++;
   }

   auto bitmap_font = _font.getCoords(command);
   _font.draw(window, bitmap_font, offset_x, h_screen - 28);

   // draw cursor
   const auto elapsed = GlobalClock::getInstance().getElapsedTime();
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
   updateInventoryItems();
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
   }

   if (!_next_animation_duration_stamina.has_value())
   {
      _next_animation_duration_stamina = getRandomDuration(_animation_stamina_duration_range[0], _animation_stamina_duration_range[1]);
   }

   if (!_next_animation_duration_skull_blink.has_value())
   {
      _next_animation_duration_skull_blink =
         getRandomDuration(_animation_skull_blink_duration_range[0], _animation_skull_blink_duration_range[1]);
   }

   if (_animation_duration_heart > _next_animation_duration_heart)
   {
      _animation_duration_heart = HighResDuration::zero();
      _animation_heart->seekToStart();
      _animation_heart->play();
   }
   else
   {
      _animation_heart->update(dt);
   }

   if (_animation_duration_stamina > _next_animation_duration_stamina)
   {
      _animation_duration_stamina = HighResDuration::zero();
      _animation_stamina->seekToStart();
      _animation_stamina->play();
   }
   else
   {
      _animation_stamina->update(dt);
   }

   if (_animation_duration_skull_blink > _next_animation_duration_skull_blink)
   {
      _animation_duration_skull_blink = HighResDuration::zero();
      _animation_skull_blink->seekToStart();
      _animation_skull_blink->play();
   }
   else
   {
      _animation_skull_blink->update(dt);
   }

   if (_animation_heart->_finished)
   {
      _next_animation_duration_heart.reset();
   }

   if (_animation_stamina->_finished)
   {
      _next_animation_duration_stamina.reset();
   }

   if (_animation_skull_blink->_finished)
   {
      _next_animation_duration_skull_blink.reset();
   }

   const auto elapsed_micros = dt.asMicroseconds();
   HighResDuration elapsed_duration = std::chrono::microseconds(elapsed_micros);

   _animation_duration_heart += elapsed_duration;
   _animation_duration_stamina += elapsed_duration;
   _animation_duration_skull_blink += elapsed_duration;
}

void InfoLayer::drawHeartAnimation(sf::RenderTarget& window, sf::RenderStates states)
{
   _heart_animation.draw(window, states);
}

void InfoLayer::drawInventoryItem(sf::RenderTarget& window, sf::RenderStates states)
{
   const auto& inventory = SaveState::getPlayerInfo()._inventory;

   for (auto i = 0u; i < inventory._slots.size(); i++)
   {
      const auto slot = inventory._slots[i];

      if (slot.empty())
      {
         continue;
      }

      window.draw(_inventory_sprites[i]);
      _slot_item_layers[i]->draw(window, states);
   }
}
