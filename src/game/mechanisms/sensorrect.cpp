#include "sensorrect.h"
#include <ranges>
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tools/log.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"
#include "game/player/player.h"

namespace
{
const auto registered_sensorrect = []
{
   auto& registry = GameMechanismDeserializerRegistry::instance();
   registry.mapGroupToLayer("SensorRect", "sensor_rects");

   registry.registerLayerName(
      "sensor_rects",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<SensorRect>(parent);
         mechanism->setup(data);
         mechanisms["sensor_rects"]->push_back(mechanism);
      }
   );
   registry.registerObjectGroup(
      "SensorRect",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<SensorRect>(parent);
         mechanism->setup(data);
         mechanisms["sensor_rects"]->push_back(mechanism);
      }
   );
   return true;
}();
}  // namespace

SensorRect::SensorRect(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(GameNode).name());
}

void SensorRect::update(const sf::Time& /*dt*/)
{
   const auto player_intersects = Player::getCurrent()->getPixelRectFloat().findIntersection(_rect).has_value();

   if (player_intersects)
   {
      if (!_player_intersects)
      {
         // trigger mechanism when player enters the rect
         if (_event == Event::OnEnter)
         {
            processAction();
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
            processAction();
         }
      }
   }

   _player_intersects = player_intersects;
}

std::optional<sf::FloatRect> SensorRect::getBoundingBoxPx()
{
   return _rect;
}

void SensorRect::setup(const GameDeserializeData& data)
{
   setObjectId(data._tmx_object->_name);
   _rect = sf::FloatRect{{data._tmx_object->_x_px, data._tmx_object->_y_px}, {data._tmx_object->_width_px, data._tmx_object->_height_px}};

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
   if (!_reference_id.has_value())
   {
      return;
   }

   auto filtered_view = mechanisms | std::views::filter(
                                        [this](const auto& object)
                                        {
                                           auto* game_node = dynamic_cast<GameNode*>(object.get());
                                           return (game_node && game_node->getObjectId() == _reference_id);
                                        }
                                     );

   _references.insert(_references.end(), filtered_view.begin(), filtered_view.end());
}

void SensorRect::addSensorCallback(const SensorCallback& callback)
{
   _callbacks.push_back(callback);
}

bool SensorRect::playerIntersects() const
{
   return _player_intersects;
}

void SensorRect::processAction()
{
   switch (_action)
   {
      case Action::Disable:
      {
         for (auto& ref : _references)
         {
            ref->setEnabled(false);
         }
         break;
      }
      case Action::Enable:
      {
         for (auto& ref : _references)
         {
            ref->setEnabled(true);
         }
         break;
      }
      case Action::Toggle:
      {
         for (auto& ref : _references)
         {
            ref->setEnabled(!ref->isEnabled());
         }
         break;
      }
   }

   for (auto& callback : _callbacks)
   {
      callback(_object_id);
   }
}
