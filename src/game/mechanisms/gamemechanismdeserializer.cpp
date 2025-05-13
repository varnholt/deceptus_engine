#include "gamemechanismdeserializer.h"
#include "gamemechanismdeserializerconstants.h"

#include "framework/tmxparser/tmxparser.h"
#include "framework/tools/log.h"

#include "game/mechanisms/bubblecube.h"
#include "game/mechanisms/checkpoint.h"
#include "game/mechanisms/collapsingplatform.h"
#include "game/mechanisms/controllerhelp.h"
#include "game/mechanisms/conveyorbelt.h"
#include "game/mechanisms/crusher.h"
#include "game/mechanisms/deathblock.h"
#include "game/mechanisms/dialogue.h"
#include "game/mechanisms/door.h"
#include "game/mechanisms/dust.h"
#include "game/mechanisms/extra.h"
#include "game/mechanisms/fan.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"
#include "game/mechanisms/interactionhelp.h"
#include "game/mechanisms/laser.h"
#include "game/mechanisms/lever.h"
#include "game/mechanisms/levermechanismmerger.h"
#include "game/mechanisms/moveablebox.h"
#include "game/mechanisms/movingplatform.h"
#include "game/mechanisms/onoffblock.h"
#include "game/mechanisms/portal.h"
#include "game/mechanisms/rope.h"
#include "game/mechanisms/sensorrect.h"
#include "game/mechanisms/shaderlayer.h"
#include "game/mechanisms/smokeeffect.h"
#include "game/mechanisms/soundemitter.h"
#include "game/mechanisms/spikeball.h"
#include "game/mechanisms/spikeblock.h"
#include "game/mechanisms/spikes.h"
#include "game/mechanisms/staticlight.h"
#include "game/mechanisms/textlayer.h"
#include "game/mechanisms/watersurface.h"
#include "game/mechanisms/weather.h"

#include <ranges>

void GameMechanismDeserializer::deserialize(
   const TmxParser& tmx_parser,
   GameNode* parent,
   const GameDeserializeData& data_ref,
   std::unordered_map<std::string, std::vector<std::shared_ptr<GameMechanism>>*>& mechanisms
)
{
   // clear all previously created internal data it's not cleaned up, even if all instances are deleted
   Laser::resetAll();
   Fan::resetAll();

   GameDeserializeData data(data_ref);

   auto* mechanism_blocking_rects = mechanisms[std::string{layer_name_blocking_rects}];
   auto* mechanism_bouncers = mechanisms[std::string{layer_name_bouncers}];
   auto* mechanism_box_colliders = mechanisms[std::string{layer_name_box_colliders}];
   auto* mechanism_bubble_cubes = mechanisms[std::string{layer_name_bubble_cube}];
   auto* mechanism_button_rects = mechanisms[std::string{layer_name_button_rects}];
   auto* mechanism_checkpoints = mechanisms[std::string{layer_name_checkpoints}];
   auto* mechanism_collapsing_platforms = mechanisms[std::string{layer_name_collapsing_platforms}];
   auto* mechanism_controller_help = mechanisms[std::string{layer_name_controller_help}];
   auto* mechanism_conveyor_belts = mechanisms[std::string{layer_name_conveyorbelts}];
   auto* mechanism_crushers = mechanisms[std::string{layer_name_crushers}];
   auto* mechanism_damage_rects = mechanisms[std::string{layer_name_damage_rects}];
   auto* mechanism_death_blocks = mechanisms[std::string{layer_name_death_blocks}];
   auto* mechanism_dialogues = mechanisms[std::string{layer_name_dialogues}];
   auto* mechanism_doors = mechanisms[std::string{layer_name_doors}];
   auto* mechanism_dust = mechanisms[std::string{layer_name_dust}];
   auto* mechanism_extras = mechanisms[std::string{layer_name_extras}];
   auto* mechanism_enemy_walls = mechanisms[std::string{layer_name_enemy_walls}];
   auto* mechanism_fans = mechanisms[std::string{layer_name_fans}];
   auto* mechanism_fireflies = mechanisms[std::string{layer_name_fireflies}];
   auto* mechanism_info_overlays = mechanisms[std::string{layer_name_info_overlays}];
   auto* mechanism_interaction_help = mechanisms[std::string{layer_name_interaction_help}];
   auto* mechanism_lasers = mechanisms[std::string{layer_name_lasers}];
   auto* mechanism_levers = mechanisms[std::string{layer_name_levers}];
   auto* mechanism_moveable_objects = mechanisms[std::string{layer_name_moveable_objects}];
   auto* mechanism_on_off_blocks = mechanisms[std::string{layer_name_on_off_blocks}];
   auto* mechanism_platforms = mechanisms[std::string{layer_name_platforms}];
   auto* mechanism_portals = mechanisms[std::string{layer_name_portals}];
   auto* mechanism_ropes = mechanisms[std::string{layer_name_ropes}];
   auto* mechanism_rotating_blades = mechanisms[std::string{layer_name_rotating_blades}];
   auto* mechanism_sensor_rects = mechanisms[std::string{layer_name_sensor_rects}];
   auto* mechanism_shader_quads = mechanisms[std::string{layer_name_shader_quads}];
   auto* mechanism_smoke_effect = mechanisms[std::string{layer_name_smoke_effect}];
   auto* mechanism_sound_emitters = mechanisms[std::string{layer_name_sound_emitters}];
   auto* mechanism_spike_balls = mechanisms[std::string{layer_name_spike_balls}];
   auto* mechanism_spike_blocks = mechanisms[std::string{layer_name_spike_blocks}];
   auto* mechanism_spikes = mechanisms[std::string{layer_name_interval_spikes}];
   auto* mechanism_static_lights = mechanisms[std::string{layer_name_static_lights}];
   auto* mechanism_text_layers = mechanisms[std::string{layer_name_text_layer}];
   auto* mechanism_treasure_chests = mechanisms[std::string{layer_name_treasure_chests}];
   auto* mechanism_water_damage = mechanisms[std::string{layer_name_water_damage}];
   auto* mechanism_weather = mechanisms[std::string{layer_name_weather}];
   auto* mechanism_water_surface = mechanisms[std::string{layer_name_weather}];
   auto* mechanism_zoom_rects = mechanisms[std::string{layer_name_zoom_rects}];

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

            if (auto initializer_function = registry.getForLayer(layer->_name))
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
                  const auto& obj = pair.second;
                  data._tmx_object = obj;
                  data._tmx_object_group = group;

                  const auto& key = obj->_template_type.value_or(group->_name);

                  if (auto initializer_function = registry.getForTemplateType(key))
                  {
                     std::invoke(*initializer_function, parent, data, mechanisms);
                  }
               }
            );
         }
      }
   );

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

         if (layer->_name == layer_name_fans)
         {
            Fan::load(data);
         }
         else if (layer->_name == layer_name_lasers_v1)
         {
            const auto mechanism = Laser::load(parent, data);
            mechanism_lasers->insert(mechanism_lasers->end(), mechanism.begin(), mechanism.end());
         }
         else if (layer->_name == layer_name_lasers_v2 || layer->_name == layer_name_lasers)  // support for dstar's new laser tileset
         {
            const auto mechanism = Laser::load(parent, data);
            mechanism_lasers->insert(mechanism_lasers->end(), mechanism.begin(), mechanism.end());
         }
         else if (layer->_name == layer_name_platforms)
         {
            *mechanism_platforms = MovingPlatform::load(parent, data);
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
            else if (object_group->_name == layer_name_platform_paths || tmx_object->_template_type == type_name_platform_path)
            {
               MovingPlatform::link(*mechanism_platforms, data);
            }
            else if (object_group->_name == layer_name_platforms || tmx_object->_template_type == type_name_platform)
            {
               MovingPlatform::deserialize(tmx_object);
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
   Fan::merge();
   WaterSurface::merge();
   *mechanism_fans = Fan::getFans();
   *mechanism_platforms = MovingPlatform::merge(parent, data);

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
