#include "gamemechanismregistry.h"

#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanismdeserializerconstants.h"

#include <ranges>
#include <regex>

GameMechanismRegistry::GameMechanismRegistry()
{
   _mechanisms_list = {
      &_mechanism_blocking_rects,
      &_mechanism_bouncers,
      &_mechanism_box_colliders,
      &_mechanism_bubble_cubes,
      &_mechanism_button_rects,
      &_mechanism_checkpoints,
      &_mechanism_collapsing_platforms,
      &_mechanism_controller_help,
      &_mechanism_conveyor_belts,
      &_mechanism_crushers,
      &_mechanism_damage_rects,
      &_mechanism_death_blocks,
      &_mechanism_destructible_blocking_rects,
      &_mechanism_dialogues,
      &_mechanism_doors,
      &_mechanism_dust,
      &_mechanism_enemy_walls,
      &_mechanism_extras,
      &_mechanism_fans,
      &_mechanism_fireflies,
      &_mechanism_gateways,
      &_mechanism_info_overlay,
      &_mechanism_interaction_help,
      &_mechanism_lasers,
      &_mechanism_levers,
      &_mechanism_moveable_boxes,
      &_mechanism_on_off_blocks,
      &_mechanism_platforms,
      &_mechanism_portals,
      &_mechanism_ropes,
      &_mechanism_rotating_blades,
      &_mechanism_sensor_rects,
      &_mechanism_shader_layers,
      &_mechanism_sound_emitters,
      &_mechanism_smoke_effect,
      &_mechanism_spike_balls,
      &_mechanism_spike_blocks,
      &_mechanism_spikes,
      &_mechanism_static_lights,
      &_mechanism_text_layers,
      &_mechanism_treasure_chests,
      &_mechanism_water_damage,
      &_mechanism_water_surface,
      &_mechanism_weather,
      &_mechanism_wind,
      &_mechanism_zoomrects,
   };

   _mechanisms_map[std::string{layer_name_blocking_rects}] = &_mechanism_blocking_rects;
   _mechanisms_map[std::string{layer_name_bouncers}] = &_mechanism_bouncers;
   _mechanisms_map[std::string{layer_name_box_colliders}] = &_mechanism_box_colliders;
   _mechanisms_map[std::string{layer_name_bubble_cube}] = &_mechanism_bubble_cubes;
   _mechanisms_map[std::string{layer_name_button_rects}] = &_mechanism_button_rects;
   _mechanisms_map[std::string{layer_name_checkpoints}] = &_mechanism_checkpoints;
   _mechanisms_map[std::string{layer_name_collapsing_platforms}] = &_mechanism_collapsing_platforms;
   _mechanisms_map[std::string{layer_name_controller_help}] = &_mechanism_controller_help;
   _mechanisms_map[std::string{layer_name_conveyorbelts}] = &_mechanism_conveyor_belts;
   _mechanisms_map[std::string{layer_name_crushers}] = &_mechanism_crushers;
   _mechanisms_map[std::string{layer_name_damage_rects}] = &_mechanism_damage_rects;
   _mechanisms_map[std::string{layer_name_death_blocks}] = &_mechanism_death_blocks;
   _mechanisms_map[std::string{layer_name_destructible_blocking_rects}] = &_mechanism_destructible_blocking_rects;
   _mechanisms_map[std::string{layer_name_dialogues}] = &_mechanism_dialogues;
   _mechanisms_map[std::string{layer_name_doors}] = &_mechanism_doors;
   _mechanisms_map[std::string{layer_name_dust}] = &_mechanism_dust;
   _mechanisms_map[std::string{layer_name_enemy_walls}] = &_mechanism_enemy_walls;
   _mechanisms_map[std::string{layer_name_extras}] = &_mechanism_extras;
   _mechanisms_map[std::string{layer_name_fans}] = &_mechanism_fans;
   _mechanisms_map[std::string{layer_name_fireflies}] = &_mechanism_fireflies;
   _mechanisms_map[std::string{layer_name_gateways}] = &_mechanism_gateways;
   _mechanisms_map[std::string{layer_name_info_overlays}] = &_mechanism_info_overlay;
   _mechanisms_map[std::string{layer_name_interaction_help}] = &_mechanism_interaction_help;
   _mechanisms_map[std::string{layer_name_interval_spikes}] = &_mechanism_spikes;
   _mechanisms_map[std::string{layer_name_lasers}] = &_mechanism_lasers;
   _mechanisms_map[std::string{layer_name_levers}] = &_mechanism_levers;
   _mechanisms_map[std::string{layer_name_moveable_objects}] = &_mechanism_moveable_boxes;
   _mechanisms_map[std::string{layer_name_on_off_blocks}] = &_mechanism_on_off_blocks;
   _mechanisms_map[std::string{layer_name_platforms}] = &_mechanism_platforms;
   _mechanisms_map[std::string{layer_name_portals}] = &_mechanism_portals;
   _mechanisms_map[std::string{layer_name_ropes}] = &_mechanism_ropes;
   _mechanisms_map[std::string{layer_name_rotating_blades}] = &_mechanism_rotating_blades;
   _mechanisms_map[std::string{layer_name_sensor_rects}] = &_mechanism_sensor_rects;
   _mechanisms_map[std::string{layer_name_shader_quads}] = &_mechanism_shader_layers;
   _mechanisms_map[std::string{layer_name_smoke_effect}] = &_mechanism_smoke_effect;
   _mechanisms_map[std::string{layer_name_sound_emitters}] = &_mechanism_sound_emitters;
   _mechanisms_map[std::string{layer_name_spike_balls}] = &_mechanism_spike_balls;
   _mechanisms_map[std::string{layer_name_spike_blocks}] = &_mechanism_spike_blocks;
   _mechanisms_map[std::string{layer_name_static_lights}] = &_mechanism_static_lights;
   _mechanisms_map[std::string{layer_name_text_layer}] = &_mechanism_text_layers;
   _mechanisms_map[std::string{layer_name_treasure_chests}] = &_mechanism_treasure_chests;
   _mechanisms_map[std::string{layer_name_water_damage}] = &_mechanism_water_damage;
   _mechanisms_map[std::string{layer_name_water_surface}] = &_mechanism_water_surface;
   _mechanisms_map[std::string{layer_name_weather}] = &_mechanism_weather;
   _mechanisms_map[std::string{layer_name_wind}] = &_mechanism_wind;
   _mechanisms_map[std::string{layer_name_zoom_rects}] = &_mechanism_zoomrects;
}

void GameMechanismRegistry::resetDoors()
{
   for (auto& door : _mechanism_doors)
   {
      door.reset();
   }
}

void GameMechanismRegistry::addImageLayer(const std::shared_ptr<ImageLayer>& image_layer)
{
   _image_layers.push_back(image_layer);
}

std::vector<std::shared_ptr<ImageLayer>> GameMechanismRegistry::getImageLayers() const
{
   return _image_layers;
}

std::vector<GameMechanismRegistry::MechanismVector*>& GameMechanismRegistry::getList()
{
   return _mechanisms_list;
}

GameMechanismRegistry::MechanismVectorMap& GameMechanismRegistry::getMap()
{
   return _mechanisms_map;
}

const GameMechanismRegistry::MechanismVector& GameMechanismRegistry::getDoors() const
{
   return _mechanism_doors;
}

const GameMechanismRegistry::MechanismVector& GameMechanismRegistry::getExtras() const
{
   return _mechanism_extras;
}

const GameMechanismRegistry::MechanismVector& GameMechanismRegistry::getBouncers() const
{
   return _mechanism_bouncers;
}

const GameMechanismRegistry::MechanismVector& GameMechanismRegistry::getPortals() const
{
   return _mechanism_portals;
}

const GameMechanismRegistry::MechanismVector& GameMechanismRegistry::getCheckpoints() const
{
   return _mechanism_checkpoints;
}

GameMechanismRegistry::MechanismVector GameMechanismRegistry::searchMechanismsIf(const MechanismPredicate& predicate) const
{
   // filter mechanisms by provided function
   return _mechanisms_map |
          std::views::transform(
             [&predicate](auto& kv)
             {
                const std::string_view group_key{kv.first};
                const auto& mechanism_vector = *kv.second;
                return mechanism_vector | std::views::filter([&predicate, group_key](const std::shared_ptr<GameMechanism>& mechanism)
                                                             { return predicate(mechanism, group_key); });
             }
          ) |
          std::views::join | std::ranges::to<GameMechanismRegistry::MechanismVector>();
}

GameMechanismRegistry::MechanismVector
GameMechanismRegistry::searchMechanisms(const std::string& regex_pattern, const std::optional<std::string>& group)
{
   GameMechanismRegistry::MechanismVector results;

   std::regex pattern(regex_pattern);
   for (const auto& [key, mechanism_vector] : _mechanisms_map)
   {
      // filter by mechanism group if requested
      if (group.has_value() && group.value() != key)
      {
         continue;
      }

      for (const auto& mechanism : *mechanism_vector)
      {
         auto node = std::dynamic_pointer_cast<GameNode>(mechanism);
         if (std::regex_match(node->getObjectId(), pattern))
         {
            results.push_back(mechanism);
         }
      }
   }

   // also scan image layers since those are separate at the moment
   if (!group.has_value() || group.value() == "imagelayers")
   {
      for (const auto& image_layer : _image_layers)
      {
         if (std::regex_match(image_layer->getObjectId(), pattern))
         {
            results.push_back(image_layer);
         }
      }
   }

   return results;
}
