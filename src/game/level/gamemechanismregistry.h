#ifndef GAMEMECHANISMREGISTRY_H
#define GAMEMECHANISMREGISTRY_H

#include <memory>
#include <vector>

#include "game/mechanisms/gamemechanism.h"
#include "game/mechanisms/imagelayer.h"

class GameMechanismRegistry
{
public:
   GameMechanismRegistry();

   using MechanismVector = std::vector<std::shared_ptr<GameMechanism>>;
   using MechanismVectorMap = std::unordered_map<std::string, MechanismVector*>;

   MechanismVectorMap& getMap();
   std::vector<MechanismVector*>& getList();

   const MechanismVector& getExtras() const;
   const MechanismVector& getDoors() const;
   const MechanismVector& getBouncers() const;
   const MechanismVector& getPortals() const;
   const MechanismVector& getCheckpoints() const;
   std::vector<std::shared_ptr<ImageLayer>> getImageLayers() const;

   void resetDoors();
   void addImageLayer(const std::shared_ptr<ImageLayer>& image_layer);

   std::vector<std::shared_ptr<GameMechanism>>
   searchMechanisms(const std::string& regexp, const std::optional<std::string>& group = std::nullopt);

   using MechanismPredicate = std::function<bool(const std::shared_ptr<GameMechanism>&, std::string_view)>;
   std::vector<std::shared_ptr<GameMechanism>> searchMechanismsIf(const MechanismPredicate& predicate) const;

private:
   MechanismVectorMap _mechanisms_map;
   std::vector<MechanismVector*> _mechanisms_list;
   MechanismVector _mechanism_blocking_rects;
   MechanismVector _mechanism_bouncers;
   MechanismVector _mechanism_box_colliders;
   MechanismVector _mechanism_bubble_cubes;
   MechanismVector _mechanism_button_rects;
   MechanismVector _mechanism_checkpoints;
   MechanismVector _mechanism_collapsing_platforms;
   MechanismVector _mechanism_controller_help;
   MechanismVector _mechanism_conveyor_belts;
   MechanismVector _mechanism_crushers;
   MechanismVector _mechanism_damage_rects;
   MechanismVector _mechanism_death_blocks;
   MechanismVector _mechanism_destructible_blocking_rects;
   MechanismVector _mechanism_dialogues;
   MechanismVector _mechanism_doors;
   MechanismVector _mechanism_dust;
   MechanismVector _mechanism_enemy_walls;
   MechanismVector _mechanism_extras;
   MechanismVector _mechanism_fans;
   MechanismVector _mechanism_fireflies;
   MechanismVector _mechanism_gateways;
   MechanismVector _mechanism_info_overlay;
   MechanismVector _mechanism_interaction_help;
   MechanismVector _mechanism_lasers;
   MechanismVector _mechanism_levers;
   MechanismVector _mechanism_moveable_boxes;
   MechanismVector _mechanism_on_off_blocks;
   MechanismVector _mechanism_platforms;
   MechanismVector _mechanism_portals;
   MechanismVector _mechanism_ropes;
   MechanismVector _mechanism_rotating_blades;
   MechanismVector _mechanism_sensor_rects;
   MechanismVector _mechanism_shader_layers;
   MechanismVector _mechanism_smoke_effect;
   MechanismVector _mechanism_sound_emitters;
   MechanismVector _mechanism_spike_balls;
   MechanismVector _mechanism_spike_blocks;
   MechanismVector _mechanism_spikes;
   MechanismVector _mechanism_text_layers;
   MechanismVector _mechanism_treasure_chests;
   MechanismVector _mechanism_static_lights;
   MechanismVector _mechanism_water_damage;
   MechanismVector _mechanism_water_surface;
   MechanismVector _mechanism_weather;
   MechanismVector _mechanism_wind;
   MechanismVector _mechanism_zoomrects;

   std::vector<std::shared_ptr<ImageLayer>> _image_layers;
};

#endif  // GAMEMECHANISMREGISTRY_H
