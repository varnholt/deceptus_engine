#pragma once

#include <chrono>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

#include "framework/tmxparser/tmxobject.h"
#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"

struct ScreenTransition;

/// \brief camera-limiting room made of one or more rectangular sub-rooms.
struct Room : std::enable_shared_from_this<Room>, public GameNode
{
   /// \brief returns a shared pointer to this room.
   /// \return shared pointer created via shared_from_this.
   std::shared_ptr<Room> getptr()
   {
      return shared_from_this();
   }

   /// \brief transition effect played when entering a room.
   enum class TransitionEffect
   {
      FadeOutFadeIn
   };

   /// \brief entry strip inside a sub-room that detects from which side the player entered.
   struct RoomEnterArea
   {
      /// \brief reads enter-area rectangle and optional start overrides from TMX data.
      /// \param data deserialize context containing object geometry and properties.
      void deserializeEnterArea(const GameDeserializeData& data);
      std::string _name;
      sf::FloatRect _rect;
      std::optional<sf::Vector2i> _start_position;
      std::optional<sf::Vector2i> _start_offset;
   };

   /// \brief single rectangular section of a room group.
   struct SubRoom
   {
      /// \brief finds the first entry strip containing the player position.
      /// \param player_pos_px player position in pixels.
      /// \return matching entry strip, or std::nullopt when none contains the point.
      std::optional<RoomEnterArea> findEnteredArea(const sf::Vector2f& player_pos_px) const;
      /// \brief initializes sub-room rectangle and default entry strips from TMX data.
      /// \param data deserialize context containing object geometry and properties.
      void deserialize(const GameDeserializeData& data);

      std::string _name;
      sf::FloatRect _rect;
      std::vector<RoomEnterArea> _enter_areas;
   };

   /// \brief creates a room node and assigns a unique room id.
   /// \param parent parent node in the scene graph.
   Room(GameNode* parent);

   /// \brief deserializes one TMX room object into grouped room instances.
   /// \param parent parent node for newly created rooms.
   /// \param data deserialize context containing current TMX object.
   /// \param rooms destination list of rooms to create or extend.
   static void deserialize(GameNode* parent, const GameDeserializeData& data, std::vector<std::shared_ptr<Room>>& rooms);
   /// \brief finds the first room whose sub-room contains a point.
   /// \param p query point in pixels.
   /// \param rooms candidate rooms.
   /// \return first matching room, or nullptr when no room contains the point.
   static std::shared_ptr<Room> find(const sf::Vector2f& p, const std::vector<std::shared_ptr<Room>>& rooms);
   /// \brief finds the first room whose sub-room intersects a rectangle.
   /// \param p query rectangle in pixels.
   /// \param rooms candidate rooms.
   /// \return first matching room, or nullptr when no intersection exists.
   static std::shared_ptr<Room> find(const sf::FloatRect& p, const std::vector<std::shared_ptr<Room>>& rooms);
   /// \brief finds all rooms whose sub-rooms intersect a rectangle.
   /// \param p query rectangle in pixels.
   /// \param rooms candidate rooms.
   /// \return all matching rooms.
   static std::vector<std::shared_ptr<Room>> findAll(const sf::FloatRect& p, const std::vector<std::shared_ptr<Room>>& rooms);
   /// \brief merges globally collected custom enter areas into affected sub-rooms.
   /// \param rooms rooms that receive matching enter areas.
   static void mergeEnterAreas(const std::vector<std::shared_ptr<Room>>& rooms);

   /// \brief starts configured room transition effects and related callbacks.
   void startTransition();
   /// \brief locks room updates for a configurable delay before camera reassignment.
   void lockCamera();
   /// \brief applies optional start position or offset from the entered area.
   void movePlayerToRoomStartPosition();
   /// \brief unlocks camera and synchronizes camera constraints to this room immediately.
   void syncCamera();

   /// \brief returns the sub-room currently containing the player.
   /// \param player_pos_px player position in pixels.
   /// \return matching sub-room, or std::nullopt when player is outside all sub-rooms.
   std::optional<SubRoom> activeSubRoom(const sf::Vector2f& player_pos_px) const;
   /// \brief returns the entry strip currently containing the player.
   /// \param player_pos_px player position in pixels.
   /// \return matching entry strip, or std::nullopt when none is active.
   std::optional<RoomEnterArea> enteredArea(const sf::Vector2f& player_pos_px) const;

   /// \brief finds a sub-room that contains a point.
   /// \param p query point in pixels.
   /// \return iterator to the matching sub-room or _sub_rooms.end().
   std::vector<SubRoom>::const_iterator findSubRoom(const sf::Vector2f& p) const;
   /// \brief finds a sub-room that intersects a rectangle.
   /// \param p query rectangle in pixels.
   /// \return iterator to the matching sub-room or _sub_rooms.end().
   std::vector<SubRoom>::const_iterator findSubRoom(const sf::FloatRect& p) const;
   /// \brief clamps camera coordinates to room boundaries.
   /// \param x x coordinate in pixels.
   /// \param y y coordinate in pixels.
   /// \param focus_offset horizontal look offset applied by the camera system.
   /// \param view_ratio_y vertical camera view ratio.
   /// \return true when clamping was applied.
   bool correctedCamera(float& x, float& y, float focus_offset, float view_ratio_y) const;
   /// \brief builds the fade-out/fade-in transition configured for this room.
   /// \return transition object ready to be pushed to ScreenTransitionHandler.
   std::unique_ptr<ScreenTransition> makeFadeTransition();

   std::vector<SubRoom> _sub_rooms;
   std::optional<std::string> _group_name;
   int32_t _id = 0;

   /// specify how long the camera position should not be updated after entering the room
   std::optional<std::chrono::milliseconds> _camera_lock_delay{0};

   /// whenever a room is entered, a transition effect can be shown, or none when not specified
   std::optional<TransitionEffect> _transition_effect;
   float _fade_in_speed = 2.0f;
   float _fade_out_speed = 2.0f;
   std::chrono::milliseconds _delay_between_effects_ms{250};
   bool _camera_sync_after_fade_out = true;
   bool _camera_locked = false;

private:
   /// \brief reads legacy enter-position properties from TMX and applies them to a sub-room.
   /// \param sub_room sub-room to update.
   /// \param data deserialize context containing source properties.
   void readEntracePositions(Room::SubRoom sub_room, const GameDeserializeData& data);
   /// \brief deserializes one top-level enter-area object.
   /// \param data deserialize context containing area object data.
   void deserializeEnterArea(const GameDeserializeData& data);
};
