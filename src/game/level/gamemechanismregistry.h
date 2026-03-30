#ifndef GAMEMECHANISMREGISTRY_H
#define GAMEMECHANISMREGISTRY_H

#include <memory>
#include <vector>

#include "game/mechanisms/gamemechanism.h"
#include "game/mechanisms/imagelayer.h"

/// \brief owns all mechanism groups loaded from TMX mechanism layers.
class GameMechanismRegistry
{
public:
   /// \brief builds mechanism group containers and registers their layer-name lookup map.
   GameMechanismRegistry();

   using MechanismVector = std::vector<std::shared_ptr<GameMechanism>>;
   using MechanismVectorMap = std::unordered_map<std::string, MechanismVector*>;

   /// \brief returns the mutable map from mechanism layer names to mechanism vectors.
   /// \return mechanism group map used during deserialization and lookup.
   MechanismVectorMap& getMap();

   /// \brief returns all mechanism vectors as a flat list of group pointers.
   /// \return list of mechanism groups for bulk iteration.
   std::vector<MechanismVector*>& getList();

   /// \brief returns the extra mechanism group.
   /// \return vector containing extra mechanisms.
   const MechanismVector& getExtras() const;

   /// \brief returns the door mechanism group.
   /// \return vector containing door mechanisms.
   const MechanismVector& getDoors() const;

   /// \brief returns the bouncer mechanism group.
   /// \return vector containing bouncer mechanisms.
   const MechanismVector& getBouncers() const;

   /// \brief returns the portal mechanism group.
   /// \return vector containing portal mechanisms.
   const MechanismVector& getPortals() const;

   /// \brief returns the checkpoint mechanism group.
   /// \return vector containing checkpoint mechanisms.
   const MechanismVector& getCheckpoints() const;

   /// \brief returns all non-mechanism image layers.
   /// \return copy of image layer instances loaded from TMX image layers.
   std::vector<std::shared_ptr<ImageLayer>> getImageLayers() const;

   /// \brief clears all stored door pointers.
   void resetDoors();

   /// \brief stores an image layer loaded from TMX.
   /// \param image_layer image layer instance to append.
   void addImageLayer(const std::shared_ptr<ImageLayer>& image_layer);

   /// \brief finds mechanisms by object id regular expression and optional mechanism group.
   /// \param regexp regular expression matched against GameNode object ids.
   /// \param group optional group key from the mechanism map, or "imagelayers" for image layers.
   /// \return mechanisms and image layers that match the filter criteria.
   std::vector<std::shared_ptr<GameMechanism>>
   searchMechanisms(const std::string& regexp, const std::optional<std::string>& group = std::nullopt);

   using MechanismPredicate = std::function<bool(const std::shared_ptr<GameMechanism>&, std::string_view)>;

   /// \brief returns mechanisms that satisfy a caller-provided predicate.
   /// \param predicate predicate receiving a mechanism and its group key.
   /// \return flattened list of all mechanisms for which predicate returns true.
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
