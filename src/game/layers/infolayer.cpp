#include "infolayer.h"

#include "framework/easings/easings.h"
#include "framework/image/psd.h"
#include "framework/tools/globalclock.h"
#include "framework/tools/log.h"
#include "game/animation/animationframedata.h"
#include "game/camera/camerapanorama.h"
#include "game/config/gameconfiguration.h"
#include "game/debug/console.h"
#include "game/io/texturepool.h"
#include "game/level/roomupdater.h"
#include "game/player/extratable.h"
#include "game/player/player.h"
#include "game/player/playerinfo.h"
#include "game/state/displaymode.h"
#include "game/state/savestate.h"

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

   const auto player_health_layers = {
      "1",
      "2",
      "3",
      "4",
      "5",
      "6",
      "7",
      "8",
      "9",
      "10",
      "11",
      "12",
      "13",
      "14",
      "15",
      "16",
      "17",
      "18",
      "19",
      "20",
      "21",
      "22",
      "23",
      "24",
      "25",
      "26",
      "27",
      "28",
      "29",
      "30",
      "31",
      "32",
      "33",
      "34",
      "35",
      "36",
      "37",
      "38",
      "39",
      "40",
      "hp_slot_01",
      "hp_slot_02",
      "hp_slot_03",
      "hp_slot_04",
      "hp_slot_05",
      "hp_slot_06",
      "hp_slot_07",
      "hp_slot_08",
      "hp_slot_09",
      "hp_slot_10",
      "hp_slot_11",
      "hp_slot_12",
      "hp_slot_13",
      "hp_slot_03 #1",
      "energy_1",
      "energy_2",
      "energy_3",
      "energy_4",
      "energy_5",
      "energy_6",
      "character_window",
      "weapon_none_icon",
      "item_slot_X",
      "item_slot_Y",
   };

   // load ingame psd
   PSD psd;
   psd.setColorFormat(PSD::ColorFormat::ABGR);
   psd.load("data/game/ingame_ui.psd");

   for (const auto& psd_layer : psd.getLayers())
   {
      // skip groups
      if (!psd_layer.isImageLayer())
      {
         continue;
      }

      auto layer = std::make_shared<Layer>();

      try
      {
         const auto texture_size = sf::Vector2u(psd_layer.getWidth(), psd_layer.getHeight());
         auto texture = std::make_shared<sf::Texture>(texture_size);
         texture->update(reinterpret_cast<const uint8_t*>(psd_layer.getImage().getData().data()));

         auto sprite = std::make_shared<sf::Sprite>(*texture);
         sprite->setPosition({static_cast<float>(psd_layer.getLeft()), static_cast<float>(psd_layer.getTop())});

         layer->_visible = psd_layer.isVisible();
         layer->_texture = texture;
         layer->_sprite = sprite;

         auto layer_data = std::make_shared<LayerData>(layer);
         layer_data->_pos = sprite->getPosition();
         _layers[psd_layer.getName()] = layer_data;

         // store all player health related layers
         if (std::ranges::contains(player_health_layers, psd_layer.getName()))
         {
            _player_health_layers.push_back(layer_data);
         }
      }
      catch (...)
      {
         Log::Fatal() << "failed to create texture: " << psd_layer.getName();
      }
   }

   // init heart layers
   _heart_layers.reserve(heart_quarter_layer_count);
   for (auto i = 1u; i <= heart_quarter_layer_count; i++)
   {
      _heart_layers.push_back(_layers[std::format("{}", i)]->_layer);
   }

   _stamina_layers.reserve(6);
   for (auto i = 1u; i <= 6; i++)
   {
      _stamina_layers.push_back(_layers[std::format("energy_{}", i)]->_layer);
   }

   _character_window_layer = _layers["character_window"]->_layer;

   _slot_item_layers[0] = _layers["item_slot_X"]->_layer;
   _slot_item_layers[1] = _layers["item_slot_Y"]->_layer;

   // load heart animation
   const auto heart_animation_interval_ms = sf::milliseconds(100);
   std::vector<sf::Time> ts;
   constexpr auto frame_count = 6 * 8 + 7;
   for (auto i = 0; i < frame_count; i++)
   {
      ts.push_back(heart_animation_interval_ms);
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
   _animation_loading = _animation_pool.create("loading", 300, 300, false, false);

   _animation_heart->_reset_to_first_frame = false;
   _animation_stamina->_reset_to_first_frame = false;
   _animation_skull_blink->_reset_to_first_frame = false;
   _animation_hp_unlock_left->_reset_to_first_frame = false;
   _animation_hp_unlock_right->_reset_to_first_frame = false;
   _animation_loading->_reset_to_first_frame = true;
   _animation_loading->_looped = true;

   _event_replay_playing = _layers["icon_playing"]->_layer;
   _event_replay_recording = _layers["icon_recording"]->_layer;

   loadInventoryItems();
   updateHealthLayerOffsets();
}

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
         std::unique_ptr<sf::Sprite> sprite = std::make_unique<sf::Sprite>(*_inventory_texture);
         sprite->setTextureRect(sf::IntRect({image._x_px, image._y_px}, {icon_width, icon_height}));
         _sprites[image._name] = std::move(sprite);
      }
   );

   auto inventory_item_1 = std::make_unique<sf::Sprite>(*_inventory_texture);
   auto inventory_item_2 = std::make_unique<sf::Sprite>(*_inventory_texture);
   inventory_item_1->setTextureRect({});
   inventory_item_2->setTextureRect({});
   inventory_item_1->setPosition({frame_0_pos_x_px, frame_0_pos_y_px});
   inventory_item_2->setPosition({frame_1_pos_x_px, frame_1_pos_y_px});
   _inventory_sprites[0] = std::move(inventory_item_1);
   _inventory_sprites[1] = std::move(inventory_item_2);
}

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

      if (sprite == nullptr)
      {
         Log::Fatal() << "could not find matching item description for '" << slot << "'. please edit inventory_items.json";
      }

      _inventory_sprites[i]->setTextureRect(sprite->getTextureRect());
   }
}

void InfoLayer::updateHealthLayerOffsets()
{
   // loading
   // -> move in health bar
   // not loading
   // -> move out health bar
   // if game state menu detected
   // -> reset
   if (DisplayMode::getInstance().isSet(Display::MainMenu))
   {
      _hide_time.reset();
      _show_time.reset();
      _player_health_x_offset = x_offset_hidden;
   }

   const auto now = GlobalClock::getInstance().getElapsedTime();
   constexpr auto duration_show_s = 1.0f;
   constexpr auto duration_hide_s = 1.0f;

   auto effect_elapsed = false;

   if (_hide_time.has_value())
   {
      // evaluate hide time
      const auto time_diff_s = (now - _hide_time.value()).asSeconds();
      const auto time_diff_norm = time_diff_s / duration_hide_s;
      effect_elapsed = time_diff_norm > 1.0f;
      if (!effect_elapsed)
      {
         const auto time_diff_norm_clamped = std::clamp(time_diff_norm, 0.0f, 1.0f);
         const auto time_diff_eased = Easings::easeInQuad<float>(time_diff_norm_clamped);
         _player_health_x_offset = std::min(_player_health_x_offset, time_diff_eased * x_offset_hidden);
      }
      else
      {
         _hide_time.reset();
      }
   }
   else if (_show_time.has_value())
   {
      // evaluate show time
      const auto time_diff_s = (now - _show_time.value()).asSeconds();
      const auto time_diff_norm = time_diff_s / duration_hide_s;
      effect_elapsed = time_diff_norm > 1.0f;
      if (!effect_elapsed)
      {
         const auto time_diff_norm_clamped = std::clamp(time_diff_norm, 0.0f, 1.0f);
         const auto time_diff_eased = Easings::easeOutCubic<float>(time_diff_norm_clamped);
         _player_health_x_offset = (1.0f - time_diff_eased) * x_offset_hidden;
      }
      else
      {
         _show_time.reset();
      }
   }

   if (effect_elapsed)
   {
      return;
   }

   std::ranges::for_each(
      _player_health_layers,
      [this](const std::shared_ptr<LayerData>& layer_data)
      {
         auto layer = layer_data->_layer;
         layer->_sprite->setPosition({layer_data->_pos.x + _player_health_x_offset, layer_data->_pos.y});
      }
   );

   _animation_heart->setPosition({heart_pos_x_px + _player_health_x_offset, heart_pos_y_px});
   _animation_stamina->setPosition({stamina_pos_x_px + _player_health_x_offset, stamina_pos_y_px});
   _animation_skull_blink->setPosition({skull_pos_x_px + _player_health_x_offset, skull_pos_y_px});
   _animation_hp_unlock_left->setPosition({_player_health_x_offset, 0.0f});
   _animation_hp_unlock_right->setPosition({_player_health_x_offset, 0.0f});

   _inventory_sprites[0]->setPosition({frame_0_pos_x_px + _player_health_x_offset, frame_0_pos_y_px});
   _inventory_sprites[1]->setPosition({frame_1_pos_x_px + _player_health_x_offset, frame_1_pos_y_px});
}

void InfoLayer::drawHealth(sf::RenderTarget& window, sf::RenderStates states)
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

void InfoLayer::drawCameraPanorama(sf::RenderTarget& window, sf::RenderStates states)
{
   if (CameraPanorama::getInstance().isKeyboardLookActive())
   {
      auto layer_cpan_up = _layers["cpan_up"]->_layer;
      auto layer_cpan_down = _layers["cpan_down"]->_layer;
      auto layer_cpan_left = _layers["cpan_left"]->_layer;
      auto layer_cpan_right = _layers["cpan_right"]->_layer;

      layer_cpan_up->draw(window, states);
      layer_cpan_down->draw(window, states);
      layer_cpan_left->draw(window, states);
      layer_cpan_right->draw(window, states);
   }
}

void InfoLayer::drawLoading(sf::RenderTarget& window, sf::RenderStates states)
{
   auto layer_autosave = _layers["autosave"]->_layer;
   if (layer_autosave->_visible)
   {
      layer_autosave->draw(window, states);
   }

   _animation_loading->play();
   _animation_loading->draw(window, states);
}

void InfoLayer::updateLoading(const sf::Time& dt)
{
   auto layer_autosave = _layers["autosave"]->_layer;
   layer_autosave->_visible = _loading;

   if (_loading)
   {
      const auto now = GlobalClock::getInstance().getElapsedTime();
      const auto alpha = 0.5f * (1.0f + sin(now.asSeconds() * 2.0f));
      layer_autosave->_sprite->setColor(sf::Color(255, 255, 255, static_cast<uint8_t>(alpha * 255)));
   }

   // need to fade in/fade out now instead of using the sine

   _animation_loading->update(dt);
}

void InfoLayer::drawEventReplay(sf::RenderStates states, sf::RenderTarget& window)
{
   if (_event_replay_recording->_visible)
   {
      _event_replay_recording->draw(window, states);
   }

   if (_event_replay_playing->_visible)
   {
      _event_replay_playing->draw(window, states);
   }
}

void InfoLayer::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   const auto w = GameConfiguration::getInstance()._view_width;
   const auto h = GameConfiguration::getInstance()._view_height;
   const sf::View view(sf::FloatRect({0.0f, 0.0f}, {static_cast<float>(w), static_cast<float>(h)}));
   window.setView(view);

   drawLoading(window, states);
   drawCameraPanorama(window, states);
   drawHealth(window, states);
   drawEventReplay(states, window);
}

void InfoLayer::drawDebugInfo(sf::RenderTarget& window)
{
   auto w = GameConfiguration::getInstance()._view_width;
   auto h = GameConfiguration::getInstance()._view_height;

   sf::View view(sf::FloatRect({0.0f, 0.0f}, {static_cast<float>(w), static_cast<float>(h)}));
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

   sf::View view(sf::FloatRect({0.0f, 0.0f}, {static_cast<float>(w_view), static_cast<float>(h_view)}));
   window.setView(view);

   const auto& layer_health = _layers["console"]->_layer;
   layer_health->draw(window, states);

   sf::View view_screen(sf::FloatRect({0.0f, 0.0f}, {static_cast<float>(w_screen), static_cast<float>(h_screen)}));
   window.setView(view_screen);

   // draw command history
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

   // draw console help
   y = 0;
   const auto indent = w_screen / 40;
   const auto& help = console.help();
   std::ostringstream oss;

   std::vector<std::string> sorted_topics;
   for (const auto& entry : help._help_messages)
   {
      sorted_topics.push_back(entry.first);
   }

   std::sort(sorted_topics.begin(), sorted_topics.end());
   for (const auto& topic : sorted_topics)
   {
      _font.draw(window, _font.getCoords(topic), w_screen / 2, (++y) * 14, sf::Color::Green);

      const auto& commands = help._help_messages.at(topic);
      for (const auto& command : commands)
      {
         _font.draw(window, _font.getCoords(command.description), w_screen / 2 + indent, (++y) * 14, sf::Color::White);

         for (const auto& example : command.examples)
         {
            _font.draw(window, _font.getCoords(example), w_screen / 2 + indent * 2, (++y) * 14, sf::Color::Red);
         }
      }
   }
}

void InfoLayer::setLoading(bool loading)
{
   if (_loading && !loading)
   {
      _show_time = GlobalClock::getInstance().getElapsedTime();
   }
   else if (!_loading && loading)
   {
      _hide_time = GlobalClock::getInstance().getElapsedTime();
   }

   _loading = loading;
}

void InfoLayer::updateEventReplayIcons()
{
   _event_replay_recording->_visible = DisplayMode::getInstance().isSet(Display::ReplayRecording);
   _event_replay_playing->_visible = DisplayMode::getInstance().isSet(Display::ReplayPlaying);
}

void InfoLayer::update(const sf::Time& dt)
{
   updateHealthLayerOffsets();
   updateInventoryItems();
   updateAnimations(dt);
   updateLoading(dt);
   updateEventReplayIcons();

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

      window.draw(*_inventory_sprites[i]);
      _slot_item_layers[i]->draw(window, states);
   }
}
