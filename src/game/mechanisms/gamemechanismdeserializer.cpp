#include "gamemechanismdeserializer.h"

#include <ranges>
#include "framework/tmxparser/tmxparser.h"
#include "framework/tools/log.h"
#include "game/mechanisms/fan.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"
#include "game/mechanisms/laser.h"
#include "game/mechanisms/levermechanismmerger.h"
#include "game/mechanisms/movingplatform.h"
#include "game/mechanisms/portal.h"
#include "game/mechanisms/sensorrect.h"
#include "game/mechanisms/shaderlayer.h"
#include "game/mechanisms/smokeeffect.h"
#include "game/mechanisms/soundemitter.h"
#include "game/mechanisms/spikes.h"
#include "game/mechanisms/textlayer.h"
#include "game/mechanisms/watersurface.h"
#include "game/mechanisms/weather.h"
#include "gamemechanismdeserializerconstants.h"

namespace
{
void debugOutputMechanisms(const std::unordered_map<std::string, std::vector<std::shared_ptr<GameMechanism>>*>& mechanisms)
{
   Log::Info() << "[Mechanisms content]";
   for (const auto& [key, vec] : mechanisms)
   {
      Log::Info() << "  " << key << ": " << (vec ? vec->size() : 0) << " elements";
   }
}

void debugOutputRegisteredCallbacks()
{
   Log::Info() << "[Registered deserializer callbacks]";
   const auto& registry = GameMechanismDeserializerRegistry::instance();
   const auto& layers = registry.getLayerNameMap();
   const auto& templates = registry.getObjectGroupMap();

   Log::Info() << "  Layers:";
   for (const auto& [name, _] : layers)
   {
      Log::Info() << "    " << name;
   }

   Log::Info() << "  TemplateTypes:";
   for (const auto& [name, _] : templates)
   {
      Log::Info() << "    " << name;
   }
}

}  // namespace

void GameMechanismDeserializer::deserialize(
   const TmxParser& tmx_parser,
   GameNode* parent,
   const GameDeserializeData& data_ref,
   std::unordered_map<std::string, std::vector<std::shared_ptr<GameMechanism>>*>& mechanisms
)
{
   // clear all previously created internal data it's not cleaned up, even if all instances are deleted
   Laser::resetAll();

   GameDeserializeData data(data_ref);

   auto* mechanism_conveyor_belts = mechanisms[std::string{layer_name_conveyorbelts}];
   auto* mechanism_doors = mechanisms[std::string{layer_name_doors}];
   auto* mechanism_fans = mechanisms[std::string{layer_name_fans}];
   auto* mechanism_lasers = mechanisms[std::string{layer_name_lasers}];
   auto* mechanism_levers = mechanisms[std::string{layer_name_levers}];
   auto* mechanism_on_off_blocks = mechanisms[std::string{layer_name_on_off_blocks}];
   auto* mechanism_platforms = mechanisms[std::string{layer_name_platforms}];
   auto* mechanism_portals = mechanisms[std::string{layer_name_portals}];
   auto* mechanism_rotating_blades = mechanisms[std::string{layer_name_rotating_blades}];
   auto* mechanism_sensor_rects = mechanisms[std::string{layer_name_sensor_rects}];
   auto* mechanism_shader_quads = mechanisms[std::string{layer_name_shader_quads}];
   auto* mechanism_smoke_effect = mechanisms[std::string{layer_name_smoke_effect}];
   auto* mechanism_sound_emitters = mechanisms[std::string{layer_name_sound_emitters}];
   auto* mechanism_spike_blocks = mechanisms[std::string{layer_name_spike_blocks}];
   auto* mechanism_spikes = mechanisms[std::string{layer_name_interval_spikes}];
   auto* mechanism_text_layers = mechanisms[std::string{layer_name_text_layer}];
   auto* mechanism_weather = mechanisms[std::string{layer_name_weather}];

   // suggested approach to deserialize mechanisms
   const auto& elements = tmx_parser.getElements();
   auto& registry = GameMechanismDeserializerRegistry::instance();

   std::ranges::for_each(
      elements,
      [&](const auto& element)
      {
         data._tmx_layer = nullptr;
         data._tmx_tileset = nullptr;
         data._tmx_object = nullptr;
         data._tmx_object_group = nullptr;

         if (element->_type == TmxElement::Type::TypeLayer)
         {
            auto layer = std::dynamic_pointer_cast<TmxLayer>(element);
            data._tmx_layer = layer;
            data._tmx_tileset = tmx_parser.getTileSet(layer);

            if (auto initializer_function = registry.getForLayerName(layer->_name))
            {
               std::invoke(*initializer_function, parent, data, mechanisms);
            }
         }
         else if (element->_type == TmxElement::Type::TypeObjectGroup)
         {
            auto group = std::dynamic_pointer_cast<TmxObjectGroup>(element);

            std::ranges::for_each(
               group->_objects,
               [&](const auto& pair)
               {
                  const auto& object = pair.second;
                  data._tmx_object = object;
                  data._tmx_object_group = group;

                  // Log::Info() << "obj template name: " << object->_template_name.value_or("");
                  // Log::Info() << "obj template type: " << object->_template_type.value_or("");
                  // Log::Info() << "obj gid: " << object->_gid.value_or("");
                  // Log::Info() << "obj name: " << object->_name;
                  // Log::Info() << "group name: " << group->_name;

                  // attempt to fetch template key from group name
                  const auto group_key = group->_name;
                  auto layer_key = registry.getObjectGroupName(group_key);

                  // if that fails, attempt to fetch template key from tmx object template type
                  if (!layer_key.has_value())
                  {
                     layer_key = object->_template_type;
                  }

                  if (auto initializer_function = registry.getForObjectGroup(layer_key.value_or(group_key)))
                  {
                     std::invoke(*initializer_function, parent, data, mechanisms);
                  }
                  else
                  {
                     // while not all mechanisms are ported, "this is fine"
                     // Log::Error() << "no initializer found for " << group_key;
                  }
               }
            );
         }
      }
   );

   // debugOutputRegisteredCallbacks();

   // deprecated approach to deserialize mechanisms
   for (const auto& element : tmx_parser.getElements())
   {
      data._tmx_layer = nullptr;
      data._tmx_tileset = nullptr;
      data._tmx_object = nullptr;
      data._tmx_object_group = nullptr;

      if (element->_type == TmxElement::Type::TypeLayer)
      {
         auto layer = std::dynamic_pointer_cast<TmxLayer>(element);
         auto tileset = tmx_parser.getTileSet(layer);

         data._tmx_layer = layer;
         data._tmx_tileset = tileset;

         if (layer->_name == layer_name_lasers_v1)
         {
            const auto mechanism = Laser::load(parent, data);
            mechanism_lasers->insert(mechanism_lasers->end(), mechanism.begin(), mechanism.end());
         }
         else if (layer->_name == layer_name_lasers_v2 || layer->_name == layer_name_lasers)  // support for dstar's new laser tileset
         {
            const auto mechanism = Laser::load(parent, data);
            mechanism_lasers->insert(mechanism_lasers->end(), mechanism.begin(), mechanism.end());
         }
         else if (layer->_name == layer_name_portals)
         {
            *mechanism_portals = Portal::load(parent, data);
         }
         else if (layer->_name == layer_name_toggle_spikes)
         {
            auto mechanism = Spikes::load(parent, data, Spikes::Mode::Toggled);
            mechanism_spikes->insert(mechanism_spikes->end(), mechanism.begin(), mechanism.end());
         }
         else if (layer->_name == layer_name_trap_spikes)
         {
            auto mechanism = Spikes::load(parent, data, Spikes::Mode::Trap);
            mechanism_spikes->insert(mechanism_spikes->end(), mechanism.begin(), mechanism.end());
         }
         else if (layer->_name == layer_name_interval_spikes)
         {
            auto mechanism = Spikes::load(parent, data, Spikes::Mode::Interval);
            mechanism_spikes->insert(mechanism_spikes->end(), mechanism.begin(), mechanism.end());
         }
      }
      else if (element->_type == TmxElement::Type::TypeObjectGroup)
      {
         auto object_group = std::dynamic_pointer_cast<TmxObjectGroup>(element);

         for (const auto& object : object_group->_objects)
         {
            auto tmx_object = object.second;

            data._tmx_object = tmx_object;
            data._tmx_object_group = object_group;

            if (object_group->_name == layer_name_lasers_v1 || object_group->_name == layer_name_lasers ||
                object_group->_name == layer_name_lasers_v2 || tmx_object->_template_type == type_name_laser)
            {
               Laser::addObject(tmx_object);
            }
            else if (object_group->_name == layer_name_portals || tmx_object->_template_type == type_name_portal)
            {
               Portal::link(*mechanism_portals, data);
            }
            else if (object_group->_name == layer_name_weather || tmx_object->_template_type == type_name_weather)
            {
               auto mechanism = Weather::deserialize(parent, data);
               mechanism_weather->push_back(mechanism);
            }
            else if (object_group->_name.rfind(layer_name_shader_quads, 0) == 0 || tmx_object->_template_type == type_name_shader_quad)
            {
               auto mechanism = ShaderLayer::deserialize(parent, data);
               mechanism_shader_quads->push_back(mechanism);
            }
            else if (object_group->_name == layer_name_switchable_objects || tmx_object->_template_type == type_name_switchable_object)
            {
               LeverMechanismMerger::addSearchRect(tmx_object);
            }
            else if (object_group->_name == layer_name_sound_emitters || tmx_object->_template_type == type_name_sound_emitter)
            {
               auto mechanism = SoundEmitter::deserialize(parent, data);
               mechanism_sound_emitters->push_back(mechanism);
            }
            else if (object_group->_name == layer_name_smoke_effect || tmx_object->_template_type == type_name_smoke_effect)
            {
               auto mechanism = SmokeEffect::deserialize(parent, data);
               mechanism_smoke_effect->push_back(mechanism);
            }
            else if (object_group->_name == layer_name_spikes || tmx_object->_template_type == type_name_spikes)
            {
               auto mechanism = Spikes::deserialize(parent, data);
               mechanism_spikes->push_back(mechanism);
            }
            else if (object_group->_name == layer_name_text_layer || tmx_object->_template_type == type_name_text_layer)
            {
               auto mechanism = TextLayer::deserialize(parent, data);
               mechanism_text_layers->push_back(mechanism);
            }
            else if (object_group->_name == layer_name_water_surface_emitter ||
                     tmx_object->_template_type == type_name_water_surface_emitter)
            {
               WaterSurface::addEmitter(parent, data);
            }
         }
      }
   }

   Laser::merge();
   WaterSurface::merge();

   // get a flat vector of all values
   std::vector<std::shared_ptr<GameMechanism>> all_mechanisms;
   for (auto& [keys, values] : mechanisms)
   {
      std::copy(values->begin(), values->end(), std::back_inserter(all_mechanisms));
   }

   for (auto& sensor_rect : *mechanism_sensor_rects)
   {
      std::dynamic_pointer_cast<SensorRect>(sensor_rect)->findReference(all_mechanisms);
   }

   LeverMechanismMerger::merge(
      *mechanism_levers,
      *mechanism_lasers,
      *mechanism_platforms,
      *mechanism_fans,
      *mechanism_conveyor_belts,
      *mechanism_spikes,
      *mechanism_spike_blocks,
      *mechanism_on_off_blocks,
      *mechanism_rotating_blades,
      *mechanism_doors
   );

   static auto warning_shown = false;
   if (!warning_shown)
   {
      for (auto& [k, v] : mechanisms)
      {
         for (const auto& mechanism : (*v))
         {
            if (mechanism->getZ() == 0)
            {
               Log::Info() << k << " has a mechanism with z = 0 ";
            }
         }
      }

      warning_shown = true;
   }

   // debugOutputMechanisms(mechanisms);
}

bool GameMechanismDeserializer::isLayerNameReserved(const std::string& layer_name)
{
   return (
      (layer_name.rfind(layer_name_doors, 0) == 0) || (layer_name == layer_name_fans) || (layer_name == layer_name_lasers) ||
      (layer_name == layer_name_lasers_v1) || (layer_name == layer_name_lasers_v2) || (layer_name == layer_name_levers) ||
      (layer_name == layer_name_platforms) || (layer_name == layer_name_portals) || (layer_name == layer_name_toggle_spikes) ||
      (layer_name == layer_name_trap_spikes) || (layer_name == layer_name_interval_spikes)
   );
}
