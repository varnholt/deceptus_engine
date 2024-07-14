#include "dialogue.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxtools.h"
#include "framework/tools/log.h"
#include "framework/tools/timer.h"
#include "game/io/valuereader.h"
#include "game/player/player.h"
#include "game/state/displaymode.h"
#include "game/state/gamestate.h"
#include "game/state/savestate.h"
#include "game/ui/messagebox.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

Dialogue::Dialogue(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(Dialogue).name());
}

std::shared_ptr<Dialogue> Dialogue::deserialize(GameNode* parent, const GameDeserializeData& data)
{
   auto dialogue = std::make_shared<Dialogue>(parent);
   dialogue->setObjectId(data._tmx_object->_name);

   auto properties = data._tmx_object->_properties;

   if (!properties)
   {
      Log::Error() << "dialogue object has no properties";
      return nullptr;
   }

   // parse dialogue items
   std::optional<sf::Vector2f> pos;
   std::optional<sf::Color> text_color;
   std::optional<sf::Color> background_color;
   constexpr auto message_box_count_max = 99;
   for (auto i = 0u; i < message_box_count_max; i++)
   {
      std::ostringstream oss;
      oss << std::setw(2) << std::setfill('0') << i;
      const auto item_id = oss.str();

      const auto& it_dialogue_items = properties->_map.find(item_id);

      // check if dialogue has an exact pixel position defined
      auto pos_x_it = data._tmx_object->_properties->_map.find(item_id + "_x_px");
      auto pos_y_it = data._tmx_object->_properties->_map.find(item_id + "_y_px");
      if (pos_x_it != data._tmx_object->_properties->_map.end() && pos_y_it != data._tmx_object->_properties->_map.end())
      {
         pos = {static_cast<float>(pos_x_it->second->_value_int.value()), static_cast<float>(pos_y_it->second->_value_int.value())};
      }

      // read text color
      auto text_color_it = data._tmx_object->_properties->_map.find(item_id + "_text_color");
      if (text_color_it != data._tmx_object->_properties->_map.end())
      {
         const auto rgba = TmxTools::color(text_color_it->second->_value_string.value());
         text_color = {rgba[0], rgba[1], rgba[2]};
      }

      // read background color
      auto background_color_it = data._tmx_object->_properties->_map.find(item_id + "_background_color");
      if (background_color_it != data._tmx_object->_properties->_map.end())
      {
         const auto rgba = TmxTools::color(background_color_it->second->_value_string.value());
         background_color = {rgba[0], rgba[1], rgba[2]};
      }

      if (it_dialogue_items != properties->_map.end())
      {
         DialogueItem item;
         item._pos = pos;
         item._message = (*it_dialogue_items).second->_value_string.value();
         item._text_color = text_color.value_or(item._text_color);
         item._background_color = background_color.value_or(item._background_color);
         dialogue->_dialogue_items.push_back(item);
      }
   }

   // parse other properties
   const auto open_automatically_it = properties->_map.find("open_automatically");
   if (open_automatically_it != properties->_map.end())
   {
      dialogue->_button_required = !((*open_automatically_it).second->_value_bool.value());
      dialogue->_consumed_counter = 1;
   }

   dialogue->_pause_game = ValueReader::readValue<bool>("pause_game", properties->_map).value_or(true);
   dialogue->_open_on_intersect = ValueReader::readValue<bool>("open_on_intersect", properties->_map).value_or(true);
   const auto show_delay = ValueReader::readValue<int32_t>("show_delay_ms", properties->_map);
   if (show_delay.has_value())
   {
      dialogue->_show_delay_ms = std::chrono::milliseconds{show_delay.value()};
   }

   dialogue->_pixel_rect =
      sf::FloatRect{data._tmx_object->_x_px, data._tmx_object->_y_px, data._tmx_object->_width_px, data._tmx_object->_height_px};

   return dialogue;
}

void Dialogue::update(const sf::Time& /*dt*/)
{
   if (!_enabled)
   {
      return;
   }

   // prevent 'pause visible' vs. 'dialogue visible' if both
   // are activated in the same frame.
   if (GameState::getInstance().getMode() == ExecutionMode::Paused || GameState::getInstance().getQueuedMode() == ExecutionMode::Paused)
   {
      return;
   }

   // don't open dialogues when camera panorama is active
   const auto& dm = DisplayMode::getInstance();
   if (dm.isSet(Display::CameraPanorama))
   {
      return;
   }

   // check whether up button is pressed
   // actually there could be a number of 'message box activation buttons' here but for
   // now it might be sufficient to just check for the up button
   if (_button_required && !Player::getCurrent()->getControls()->isMovingUp(0.7f))
   {
      return;
   }

   if (_consumed_counter.has_value() && _consumed_counter == 0)
   {
      return;
   }

   const auto& player_rect = Player::getCurrent()->getPixelRectFloat();
   if (_open_on_intersect && player_rect.intersects(_pixel_rect))
   {
      // message boxes might already be marked as inactive, however
      // they might still be fading out. the display mode 'modal', however
      // is only removed when the messagebox is actually fully destroyed.
      // thinking about this a little more, maybe the 'active' flag can actually
      // be removed entirely.
      if (!isActive() && !DisplayMode::getInstance().isSet(Display::Modal))
      {
         setActive(true);

         if (_show_delay_ms.has_value())
         {
            Timer::add(
               _show_delay_ms.value(), [this]() { showNext(); }, Timer::Type::Singleshot, Timer::Scope::UpdateIngame
            );
         }
         else
         {
            showNext();
         }
      }

      if (_consumed_counter.has_value())
      {
         (*_consumed_counter)--;
      }
   }
   else
   {
      setActive(false);
   }
}

std::optional<sf::FloatRect> Dialogue::getBoundingBoxPx()
{
   return _pixel_rect;
}

bool Dialogue::isActive() const
{
   return _active;
}

void Dialogue::setActive(bool active)
{
   _active = active;
}

void Dialogue::replace(std::string& str, const std::string& what, const std::string& with)
{
   auto index = str.find(what, 0);
   while (index != std::string::npos)
   {
      str.replace(index, what.size(), with);
      index = str.find(what, 0);
   }
}

void Dialogue::replaceTags(std::string& str)
{
   replace(str, "<player>", SaveState::getPlayerInfo()._name);
   replace(str, "<br>", "\n");
}

void Dialogue::showNext()
{
   if (_index == _dialogue_items.size())
   {
      _index = 0;

      // when done, mark the dialogue as inactive so it can be reactivated on button press
      setActive(false);

      // un-pause game if configured to be paused
      if (_pause_game)
      {
         GameState::getInstance().enqueueResume();
      }

      return;
   }

   if (_index == 0)
   {
      // pause game if configured
      if (_pause_game)
      {
         GameState::getInstance().enqueuePause();
      }
   }

   const auto item = _dialogue_items.at(_index);
   auto message_text = item._message;

   replaceTags(message_text);

   MessageBox::info(
      message_text,
      [this](MessageBox::Button /*b*/) { showNext(); },
      MessageBox::LayoutProperties{
         item._location,
         item._pos,
         item._background_color,
         item._text_color,
         item._animate_text,
         item._animate_text_speed,
         false,
         (_index == 0),                           // the first item has a show animation
         (_index == _dialogue_items.size() - 1),  // the last item has a hide animation
         _index < _dialogue_items.size() - 1      // whether to show 'show next' arrow
      }
   );

   _index++;
}
