#include "gamemechanismdeserializer.h"
#include "gamemechanismdeserializerconstants.h"

#include "framework/tmxparser/tmxparser.h"

#include "mechanisms/bouncer.h"
#include "mechanisms/bubblecube.h"
#include "mechanisms/checkpoint.h"
#include "mechanisms/collapsingplatform.h"
#include "mechanisms/controllerhelp.h"
#include "mechanisms/conveyorbelt.h"
#include "mechanisms/crusher.h"
#include "mechanisms/deathblock.h"
#include "mechanisms/dialogue.h"
#include "mechanisms/door.h"
#include "mechanisms/fan.h"
#include "mechanisms/laser.h"
#include "mechanisms/lever.h"
#include "mechanisms/moveablebox.h"
#include "mechanisms/movingplatform.h"
#include "mechanisms/onoffblock.h"
#include "mechanisms/portal.h"
#include "mechanisms/rope.h"
#include "mechanisms/ropewithlight.h"
#include "mechanisms/rotatingblade.h"
#include "mechanisms/sensorrect.h"
#include "mechanisms/shaderlayer.h"
#include "mechanisms/spikeball.h"
#include "mechanisms/spikeblock.h"
#include "mechanisms/spikes.h"

// move to mechanisms
#include "effects/dust.h"
#include "weather.h"

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

   auto mechanism_bouncers = mechanisms[std::string{layer_name_bouncers}];
   auto mechanism_bubble_cubes = mechanisms[std::string{layer_name_bubble_cube}];
   auto mechanism_checkpoints = mechanisms[std::string{layer_name_checkpoints}];
   auto mechanism_collapsing_platforms = mechanisms[std::string{layer_name_collapsing_platforms}];
   auto mechanism_controller_help = mechanisms[std::string{layer_name_controller_help}];
   auto mechanism_conveyor_belts = mechanisms[std::string{layer_name_conveyorbelts}];
   auto mechanism_crushers = mechanisms[std::string{layer_name_crushers}];
   auto mechanism_death_blocks = mechanisms[std::string{layer_name_death_blocks}];
   auto mechanism_dialogues = mechanisms[std::string{layer_name_dialogues}];
   auto mechanism_doors = mechanisms[std::string{layer_name_doors}];
   auto mechanism_dust = mechanisms[std::string{layer_name_dust}];
   auto mechanism_fans = mechanisms[std::string{layer_name_fans}];
   auto mechanism_lasers = mechanisms[std::string{layer_name_lasers}];
   auto mechanism_levers = mechanisms[std::string{layer_name_levers}];
   auto mechanism_moveable_objects = mechanisms[std::string{layer_name_moveable_objects}];
   auto mechanism_on_off_blocks = mechanisms[std::string{layer_name_on_off_blocks}];
   auto mechanism_platforms = mechanisms[std::string{layer_name_platforms}];
   auto mechanism_portals = mechanisms[std::string{layer_name_portals}];
   auto mechanism_ropes = mechanisms[std::string{layer_name_ropes}];
   auto mechanism_rotating_blades = mechanisms[std::string{layer_name_rotating_blades}];
   auto mechanism_sensor_rects = mechanisms[std::string{layer_name_sensor_rects}];
   auto mechanism_shader_quads = mechanisms[std::string{layer_name_shader_quads}];
   auto mechanism_spike_balls = mechanisms[std::string{layer_name_spike_balls}];
   auto mechanism_spike_blocks = mechanisms[std::string{layer_name_spike_blocks}];
   auto mechanism_spikes = mechanisms[std::string{layer_name_interval_spikes}];
   auto mechanism_weather = mechanisms[std::string{layer_name_weather}];

   for (auto element : tmx_parser.getElements())
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

         if (layer->_name.rfind(layer_name_doors, 0) == 0)
         {
            *mechanism_doors = Door::load(data);
         }
         else if (layer->_name == layer_name_fans)
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
         else if (layer->_name == layer_name_levers)
         {
            auto mechanism = Lever::load(parent, data);
            mechanism_levers->insert(mechanism_levers->end(), mechanism.begin(), mechanism.end());
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

            if (object_group->_name == layer_name_bubble_cube || tmx_object->_type == type_name_bubble_cube)
            {
               auto mechanism = std::make_shared<BubbleCube>(parent, data);
               mechanism_bubble_cubes->push_back(mechanism);
            }
            else if (object_group->_name == layer_name_lasers_v1 || object_group->_name == layer_name_lasers ||
                     object_group->_name == layer_name_lasers_v2 || tmx_object->_type == type_name_laser)
            {
               Laser::addObject(tmx_object);
            }
            else if (object_group->_name == layer_name_doors || tmx_object->_type == type_name_door)
            {
               auto mechanism = std::make_shared<Door>(parent);
               mechanism->setup(data);
               mechanism_doors->push_back(mechanism);
            }
            else if (object_group->_name == layer_name_levers || tmx_object->_type == type_name_lever)
            {
               auto mechanism = std::make_shared<Lever>(parent);
               mechanism->setup(data);
               mechanism_levers->push_back(mechanism);
            }
            else if (object_group->_name == layer_name_fans || tmx_object->_type == type_name_fan)
            {
               Fan::addObject(parent, data);
            }
            else if (object_group->_name == layer_name_portals || tmx_object->_type == type_name_portal)
            {
               Portal::link(*mechanism_portals, data);
            }
            else if (object_group->_name == layer_name_ropes || tmx_object->_type == type_name_rope)
            {
               auto mechanism = std::make_shared<Rope>(parent);
               mechanism->setup(data);
               mechanism_ropes->push_back(mechanism);
            }
            else if (object_group->_name == layer_name_ropes_with_light || tmx_object->_type == type_name_rope_with_light)
            {
               auto mechanism = std::make_shared<RopeWithLight>(parent);
               mechanism->setup(data);
               mechanism_ropes->push_back(mechanism);
            }
            else if (object_group->_name == layer_name_rotating_blades || tmx_object->_type == type_name_rotating_blade)
            {
               auto mechanism = std::make_shared<RotatingBlade>(parent);
               mechanism->setup(data);
               mechanism_rotating_blades->push_back(mechanism);
            }
            else if (object_group->_name == layer_name_spike_balls || tmx_object->_type == type_name_spike_ball)
            {
               auto mechanism = std::make_shared<SpikeBall>(parent);
               mechanism->setup(data);
               mechanism_spike_balls->push_back(mechanism);
            }
            else if (object_group->_name == layer_name_spike_blocks || tmx_object->_type == type_name_spike_block)
            {
               auto mechanism = std::make_shared<SpikeBlock>(parent);
               mechanism->setup(data);
               mechanism_spike_blocks->push_back(mechanism);
            }
            else if (object_group->_name == layer_name_on_off_blocks || tmx_object->_type == type_name_on_off_block)
            {
               auto mechanism = std::make_shared<OnOffBlock>(parent);
               mechanism->setup(data);
               mechanism_on_off_blocks->push_back(mechanism);
            }
            else if (object_group->_name == layer_name_moveable_objects || tmx_object->_type == type_name_moveable_object)
            {
               auto mechanism = std::make_shared<MoveableBox>(parent);
               mechanism->setup(data);
               mechanism_moveable_objects->push_back(mechanism);
            }
            else if (object_group->_name == layer_name_death_blocks || tmx_object->_type == type_name_death_block)
            {
               auto death_block = std::make_shared<DeathBlock>(parent);
               death_block->setup(data);
               mechanism_death_blocks->push_back(death_block);
            }
            else if (object_group->_name == layer_name_checkpoints || tmx_object->_type == type_name_checkpoint)
            {
               const auto mechanism = Checkpoint::deserialize(parent, data);
               mechanism_checkpoints->push_back(mechanism);
            }
            else if (object_group->_name == layer_name_dialogues || tmx_object->_type == type_name_dialogue)
            {
               auto mechanism = Dialogue::deserialize(parent, data);
               mechanism_dialogues->push_back(mechanism);
            }
            else if (object_group->_name == layer_name_bouncers || tmx_object->_type == type_name_bouncer)
            {
               auto mechanism = std::make_shared<Bouncer>(parent, data);
               mechanism_bouncers->push_back(mechanism);
            }
            else if (object_group->_name == layer_name_collapsing_platforms || tmx_object->_type == type_name_collapsing_platform)
            {
               auto mechanism = std::make_shared<CollapsingPlatform>(parent, data);
               mechanism_collapsing_platforms->push_back(mechanism);
            }
            else if (object_group->_name == layer_name_controller_help || tmx_object->_type == type_name_controller_help)
            {
               auto mechanism = std::make_shared<ControllerHelp>(parent);
               mechanism->deserialize(data);
               mechanism_controller_help->push_back(mechanism);
            }
            else if (object_group->_name == layer_name_conveyorbelts || tmx_object->_type == type_name_conveyor_belt)
            {
               auto mechanism = std::make_shared<ConveyorBelt>(parent, data);
               mechanism_conveyor_belts->push_back(mechanism);
            }
            else if (object_group->_name == layer_name_crushers || tmx_object->_type == type_name_crusher)
            {
               auto mechanism = std::make_shared<Crusher>(parent);
               mechanism->setup(data);
               mechanism_crushers->push_back(mechanism);
            }
            else if (object_group->_name == layer_name_platform_paths || tmx_object->_type == type_name_platform_path)
            {
               MovingPlatform::link(*mechanism_platforms, data);
            }
            else if (object_group->_name == layer_name_platforms || tmx_object->_type == type_name_platform)
            {
               MovingPlatform::deserialize(tmx_object);
            }
            else if (object_group->_name == layer_name_weather || tmx_object->_type == type_name_weather)
            {
               auto mechanism = Weather::deserialize(parent, data);
               mechanism_weather->push_back(mechanism);
            }
            else if (object_group->_name.rfind(layer_name_shader_quads, 0) == 0 || tmx_object->_type == type_name_shader_quad)
            {
               auto quad = ShaderLayer::deserialize(parent, data);
               mechanism_shader_quads->push_back(quad);
            }
            else if (object_group->_name == layer_name_dust || tmx_object->_type == type_name_dust)
            {
               auto mechanism = Dust::deserialize(parent, data);
               mechanism_dust->push_back(mechanism);
            }
            else if (object_group->_name == layer_name_switchable_objects || tmx_object->_type == type_name_switchable_object)
            {
               Lever::addSearchRect(tmx_object);
            }
            else if (object_group->_name == layer_name_sensor_rects || tmx_object->_type == type_name_sensor_rect)
            {
               auto mechanism = std::make_shared<SensorRect>(parent);
               mechanism->setup(data);
               mechanism_sensor_rects->push_back(mechanism);
            }
         }
      }
   }

   Laser::merge();
   Fan::merge();
   *mechanism_fans = Fan::getFans();
   *mechanism_platforms = MovingPlatform::merge(parent, data);

   // get a flat vector of all values
   std::vector<std::shared_ptr<GameMechanism>> all_mechanisms;
   for (auto& [keys, values] : mechanisms)
   {
      for (auto& mechanism : *values)
      {
         all_mechanisms.push_back(mechanism);
      }
   }

   for (auto& sensor_rect : *mechanism_sensor_rects)
   {
      std::dynamic_pointer_cast<SensorRect>(sensor_rect)->findReference(all_mechanisms);
   }

   Lever::merge(
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
}

bool GameMechanismDeserializer::isLayerNameReserved(const std::string& layer_name)
{
   if ((layer_name.rfind(layer_name_doors, 0) == 0) || (layer_name == layer_name_fans) || (layer_name == layer_name_lasers) || (layer_name == layer_name_lasers_v1) || (layer_name == layer_name_lasers_v2) || (layer_name == layer_name_levers) || (layer_name == layer_name_platforms) || (layer_name == layer_name_portals) || (layer_name == layer_name_toggle_spikes) || (layer_name == layer_name_trap_spikes) || (layer_name == layer_name_interval_spikes))
   {
      return true;
   }

   return false;
}
