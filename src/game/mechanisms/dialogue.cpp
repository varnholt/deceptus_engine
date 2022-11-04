#include "dialogue.h"

#include "displaymode.h"
#include "gamestate.h"
#include "messagebox.h"
#include "player/player.h"
#include "savestate.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"

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

   // parse dialogue items
   for (auto i = 0u; i < 99; i++)
   {
      std::ostringstream oss;
      oss << std::setw(2) << std::setfill('0') << i;
      const auto& it_dialogue_items = properties->_map.find(oss.str());
      if (it_dialogue_items != properties->_map.end())
      {
         DialogueItem item;
         item._message = (*it_dialogue_items).second->_value_string.value();
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

   dialogue->_pixel_rect =
      sf::FloatRect{data._tmx_object->_x_px, data._tmx_object->_y_px, data._tmx_object->_width_px, data._tmx_object->_height_px};

   return dialogue;
}

void Dialogue::update(const sf::Time& /*dt*/)
{
   // prevent 'pause visible' vs. 'dialogue visible' if both
   // are activated in the same frame.
   if (GameState::getInstance().getMode() == ExecutionMode::Paused || GameState::getInstance().getQueuedMode() == ExecutionMode::Paused)
   {
      return;
   }

   // check whether up button is pressed
   // actually there could be a number of 'message box activation buttons' here but for
   // now it might be sufficient to just check for the up button
   if (_button_required && !Player::getCurrent()->getControls()->isUpButtonPressed())
   {
      return;
   }

   if (_consumed_counter.has_value() && _consumed_counter == 0)
   {
      return;
   }

   const auto& player_rect = Player::getCurrent()->getPixelRectFloat();
   if (player_rect.intersects(_pixel_rect))
   {
      // message boxes might already be marked as inactive, however
      // they might still be fading out. the display mode 'modal', however
      // is only removed when the messagebox is actually fully destroyed.
      // thinking about this a little more, maybe the 'active' flag can actually
      // be removed entirely.
      if (!isActive() && !DisplayMode::getInstance().isSet(Display::Modal))
      {
         setActive(true);
         showNext();
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
   if (index != std::string::npos)
   {
      str.replace(index, what.size(), with);
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
      GameState::getInstance().enqueueResume();
      return;
   }

   if (_index == 0)
   {
      GameState::getInstance().enqueuePause();
   }

   const auto item = _dialogue_items.at(_index);
   auto messag_text = item._message;

   replaceTags(messag_text);

   MessageBox::info(
      messag_text,
      [this](MessageBox::Button /*b*/) { showNext(); },
      MessageBox::LayoutProperties{
         item._location,
         item._background_color,
         item._text_color,
         item._animate_text,
         item._animate_text_speed,
         false,
         (_index == 0),                          // the first item has a show animation
         (_index == _dialogue_items.size() - 1)  // the last item has a hide animation
      }
   );

   _index++;
}
