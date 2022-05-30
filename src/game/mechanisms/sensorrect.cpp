#include "sensorrect.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tools/log.h"
#include "player/player.h"


SensorRect::SensorRect(GameNode* parent)
 : GameNode(parent)
{
    setClassName(typeid(GameNode).name());
}


void SensorRect::update(const sf::Time& /*dt*/)
{
   const auto player_intersects = Player::getCurrent()->getPlayerPixelRect().intersects(_rect);

   if (player_intersects)
   {
      if (!_player_intersects)
      {
         // trigger mechanism when player enters the rect
         if (_event == Event::OnEnter)
         {
            action();
         }
      }
   }
   else
   {
      if (_player_intersects)
      {
         // trigger mechanism when player leaves the rect
         if (_event == Event::OnLeave)
         {
            action();
         }
      }
   }

   _player_intersects = player_intersects;
}


void SensorRect::setup(const GameDeserializeData& data)
{
   _rect = sf::IntRect{
      static_cast<int32_t>(data._tmx_object->_x_px),
      static_cast<int32_t>(data._tmx_object->_y_px),
      static_cast<int32_t>(data._tmx_object->_width_px),
      static_cast<int32_t>(data._tmx_object->_height_px)
   };

   if (data._tmx_object->_properties)
   {
      auto reference_it = data._tmx_object->_properties->_map.find("reference_id");
      if (reference_it != data._tmx_object->_properties->_map.end())
      {
         _reference_id = reference_it->second->_value_string.value();
      }

      auto action_it = data._tmx_object->_properties->_map.find("action");
      if (action_it != data._tmx_object->_properties->_map.end())
      {
         const auto action = action_it->second->_value_string.value();

         if (action == "enable")
         {
            _action = Action::Enable;
         }
         else if (action == "disable")
         {
            _action = Action::Disable;
         }
         else if (action == "toggle")
         {
            _action = Action::Toggle;
         }
      }

      auto event_it = data._tmx_object->_properties->_map.find("event");
      if (event_it != data._tmx_object->_properties->_map.end())
      {
         const auto event = event_it->second->_value_string.value();

         if (event == "on_enter")
         {
            _event = Event::OnEnter;
         }
         else if (event == "on_leave")
         {
            _event = Event::OnLeave;
         }
      }
   }
}


void SensorRect::findReference(const std::vector<std::shared_ptr<GameMechanism>>& mechanisms)
{
   std::copy_if(mechanisms.begin(), mechanisms.end(), std::back_inserter(_references), [this](const auto& object){
         auto game_node = dynamic_cast<GameNode*>(object.get());
         return (game_node && game_node->getObjectId() == _reference_id);
      }
   );
}


void SensorRect::action()
{
   switch (_action)
   {
      case Action::Disable:
      {
         for (auto& ref: _references)
         {
            ref->setEnabled(false);
         }
         break;
      }
      case Action::Enable:
      {
         for (auto& ref: _references)
         {
            ref->setEnabled(true);
         }
         break;
      }
      case Action::Toggle:
      {
         for (auto& ref: _references)
         {
            ref->setEnabled(!ref->isEnabled());
         }
         break;
      }
   }
}


