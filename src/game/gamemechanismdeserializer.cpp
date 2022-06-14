
#include "gamemechanismdeserializer.h"

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
#include "mechanisms/spikeblock.h"
#include "mechanisms/spikeball.h"
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
   GameDeserializeData data(data_ref);

   auto mechanism_bouncers             = mechanisms["bouncers"];
   auto mechanism_bubble_cubes         = mechanisms["bubble_cubes"];
   auto mechanism_checkpoints          = mechanisms["checkpoints"];
   auto mechanism_collapsing_platforms = mechanisms["collapsing_platforms"];
   auto mechanism_controller_help      = mechanisms["controller_help"];
   auto mechanism_conveyor_belts       = mechanisms["conveyorbelts"];
   auto mechanism_crushers             = mechanisms["crushers"];
   auto mechanism_death_blocks         = mechanisms["death_blocks"];
   auto mechanism_dialogues            = mechanisms["dialogues"];
   auto mechanism_doors                = mechanisms["doors"];
   auto mechanism_dust                 = mechanisms["dust"];
   auto mechanism_fans                 = mechanisms["fans"];
   auto mechanism_lasers               = mechanisms["lasers"];
   auto mechanism_levers               = mechanisms["levers"];
   auto mechanism_moveable_objects     = mechanisms["moveable_objects"];
   auto mechanism_on_off_blocks        = mechanisms["on_off_blocks"];
   auto mechanism_platforms            = mechanisms["platforms"];
   auto mechanism_portals              = mechanisms["portals"];
   auto mechanism_ropes                = mechanisms["ropes"];
   auto mechanism_rotating_blades      = mechanisms["rotating_blades"];
   auto mechanism_sensor_rects         = mechanisms["sensor_rects"];
   auto mechanism_shader_quads         = mechanisms["shader_quads"];
   auto mechanism_spike_balls          = mechanisms["spike_balls"];
   auto mechanism_spike_blocks         = mechanisms["spike_blocks"];
   auto mechanism_spikes               = mechanisms["spikes"];
   auto mechanism_weather              = mechanisms["weather"];

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

         if (layer->_name.rfind("doors", 0) == 0)
         {
            *mechanism_doors = Door::load(data);
         }
         else if (layer->_name == "fans")
         {
            Fan::load(data);
         }
         else if (layer->_name == "lasers")
         {
            const auto mechanism = Laser::load(parent, data);
            mechanism_lasers->insert(mechanism_lasers->end(), mechanism.begin(), mechanism.end());
         }
         else if (layer->_name == "lasers_2") // support for dstar's new laser tileset
         {
            const auto mechanism = Laser::load(parent, data);
            mechanism_lasers->insert(mechanism_lasers->end(), mechanism.begin(), mechanism.end());
         }
         else if (layer->_name == "levers")
         {
            auto mechanism = Lever::load(parent, data);
            mechanism_levers->insert(mechanism_levers->end(), mechanism.begin(), mechanism.end());
         }
         else if (layer->_name == "platforms")
         {
            *mechanism_platforms = MovingPlatform::load(parent, data);
         }
         else if (layer->_name == "portals")
         {
            *mechanism_portals = Portal::load(parent, data);
         }
         else if (layer->_name == "toggle_spikes")
         {
            auto mechanism = Spikes::load(parent, data, Spikes::Mode::Toggled);
            mechanism_spikes->insert(mechanism_spikes->end(), mechanism.begin(), mechanism.end());
         }
         else if (layer->_name == "trap_spikes")
         {
            auto mechanism = Spikes::load(parent, data, Spikes::Mode::Trap);
            mechanism_spikes->insert(mechanism_spikes->end(), mechanism.begin(), mechanism.end());
         }
         else if (layer->_name == "interval_spikes")
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

            if (object_group->_name == "bubble_cubes")
            {
               auto mechanism = std::make_shared<BubbleCube>(parent, data);
               mechanism_bubble_cubes->push_back(mechanism);
            }
            else if (object_group->_name == "lasers" || object_group->_name == "lasers_2")
            {
               Laser::addObject(tmx_object);
            }
            else if (object_group->_name == "doors" )
            {
               auto mechanism = std::make_shared<Door>(parent);
               mechanism->setup(data);
               mechanism_doors->push_back(mechanism);
            }
            else if (object_group->_name == "levers" )
            {
               auto mechanism = std::make_shared<Lever>(parent);
               mechanism->setup(data);
               mechanism_levers->push_back(mechanism);
            }
            else if (object_group->_name == "fans")
            {
               Fan::addObject(parent, data);
            }
            else if (object_group->_name == "portals")
            {
               Portal::link(*mechanism_portals, data);
            }
            else if (object_group->_name == "ropes")
            {
               auto mechanism = std::make_shared<Rope>(parent);
               mechanism->setup(data);
               mechanism_ropes->push_back(mechanism);
            }
            else if (object_group->_name == "ropes_with_light")
            {
               auto mechanism = std::make_shared<RopeWithLight>(parent);
               mechanism->setup(data);
               mechanism_ropes->push_back(mechanism);
            }
            else if (object_group->_name == "rotating_blades")
            {
               auto mechanism = std::make_shared<RotatingBlade>(parent);
               mechanism->setup(data);
               mechanism_rotating_blades->push_back(mechanism);
            }
            else if (object_group->_name == "spike_balls")
            {
               auto mechanism = std::make_shared<SpikeBall>(parent);
               mechanism->setup(data);
               mechanism_spike_balls->push_back(mechanism);
            }
            else if (object_group->_name == "spike_blocks")
            {
               auto mechanism = std::make_shared<SpikeBlock>(parent);
               mechanism->setup(data);
               mechanism_spike_blocks->push_back(mechanism);
            }
            else if (object_group->_name == "on_off_blocks")
            {
                auto mechanism = std::make_shared<OnOffBlock>(parent);
                mechanism->setup(data);
                mechanism_on_off_blocks->push_back(mechanism);
            }
            else if (object_group->_name == "moveable_objects")
            {
               auto mechanism = std::make_shared<MoveableBox>(parent);
               mechanism->setup(data);
               mechanism_moveable_objects->push_back(mechanism);
            }
            else if (object_group->_name == "death_blocks")
            {
               auto death_block = std::make_shared<DeathBlock>(parent);
               death_block->setup(data);
               mechanism_death_blocks->push_back(death_block);
            }
            else if (object_group->_name == "checkpoints")
            {
               const auto mechanism = Checkpoint::deserialize(parent, data);
               mechanism_checkpoints->push_back(mechanism);
            }
            else if (object_group->_name == "dialogues")
            {
               auto mechanism = Dialogue::deserialize(parent, data);
               mechanism_dialogues->push_back(mechanism);
            }
            else if (object_group->_name == "bouncers")
            {
               auto mechanism = std::make_shared<Bouncer>(parent, data);
               mechanism_bouncers->push_back(mechanism);
            }
            else if (object_group->_name == "collapsing_platforms")
            {
               auto mechanism = std::make_shared<CollapsingPlatform>(parent, data);
               mechanism_collapsing_platforms->push_back(mechanism);
            }
            else if (object_group->_name == "controller_help")
            {
               auto mechanism = std::make_shared<ControllerHelp>(parent);
               mechanism->deserialize(data);
               mechanism_controller_help->push_back(mechanism);
            }
            else if (object_group->_name == "conveyorbelts")
            {
               auto mechanism = std::make_shared<ConveyorBelt>(parent, data);
               mechanism_conveyor_belts->push_back(mechanism);
            }
            else if (object_group->_name == "crushers")
            {
               auto mechanism = std::make_shared<Crusher>(parent);
               mechanism->setup(data);
               mechanism_crushers->push_back(mechanism);
            }
            else if (object_group->_name == "platform_paths")
            {
               MovingPlatform::link(*mechanism_platforms, data);
            }
            else if (object_group->_name == "platforms")
            {
               MovingPlatform::deserialize(tmx_object);
            }
            else if (object_group->_name == "weather")
            {
               auto mechanism = Weather::deserialize(parent, data);
               mechanism_weather->push_back(mechanism);
            }
            else if (object_group->_name.rfind("shader_quads", 0) == 0)
            {
               auto quad = ShaderLayer::deserialize(parent, data);
               mechanism_shader_quads->push_back(quad);
            }
            else if (object_group->_name == "dust")
            {
               auto mechanism = Dust::deserialize(parent, data);
               mechanism_dust->push_back(mechanism);
            }
            else if (object_group->_name == "switchable_objects")
            {
               Lever::addSearchRect(tmx_object);
            }
            else if (object_group->_name == "sensor_rects")
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
   if (
         (layer_name.rfind("doors", 0) == 0)
      || (layer_name == "fans")
      || (layer_name == "lasers")
      || (layer_name == "lasers_2")
      || (layer_name == "levers")
      || (layer_name == "platforms")
      || (layer_name == "portals")
      || (layer_name == "toggle_spikes")
      || (layer_name == "trap_spikes")
      || (layer_name == "interval_spikes")
   )
   {
      return true;
   }

   return false;
}

