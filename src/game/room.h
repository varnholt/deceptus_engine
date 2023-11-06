#pragma once

#include <chrono>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

#include "framework/tmxparser/tmxobject.h"
#include "gamedeserializedata.h"
#include "gamenode.h"

struct ScreenTransition;

/*! \brief Room representation used by the camera system
 *         The room class limits the camera system to a defined area.
 *
 *  A room can be defined as one or more rectangles. When configured accordingly,
 *  there can be animated transitions between rooms.
 */
struct Room : std::enable_shared_from_this<Room>, public GameNode
{
   std::shared_ptr<Room> getptr()
   {
      return shared_from_this();
   }

   enum class TransitionEffect
   {
      FadeOutFadeIn
   };

   enum class EnteredDirection : char
   {
      Invalid = '?',
      Left = 'l',
      Right = 'r',
      Top = 't',
      Bottom = 'b'
   };

   struct RoomEnterArea
   {
      void deserializeEnterArea(const GameDeserializeData& data);
      std::string _name;
      sf::FloatRect _rect;
      std::optional<sf::Vector2i> _start_position;
      std::optional<sf::Vector2i> _start_offset;
   };

   struct SubRoom
   {
      std::optional<RoomEnterArea> findEnteredArea(const sf::Vector2f& player_pos_px) const;
      void readEntracePositions(const GameDeserializeData& data);

      sf::FloatRect _rect;
      std::vector<RoomEnterArea> _enter_areas;
   };

   Room(GameNode* parent);

   static void deserialize(GameNode* parent, const GameDeserializeData& data, std::vector<std::shared_ptr<Room>>& rooms);
   static std::shared_ptr<Room> find(const sf::Vector2f& p, const std::vector<std::shared_ptr<Room>>& rooms);
   static std::shared_ptr<Room> find(const sf::FloatRect& p, const std::vector<std::shared_ptr<Room>>& rooms);
   static std::vector<std::shared_ptr<Room>> findAll(const sf::FloatRect& p, const std::vector<std::shared_ptr<Room>>& rooms);
   static void mergeStartAreas(const std::vector<std::shared_ptr<Room>>& rooms);

   void startTransition();
   void lockCamera();
   void movePlayerToRoomStartPosition();
   void syncCamera();

   std::optional<SubRoom> activeSubRoom(const sf::Vector2f& player_pos_px) const;
   std::optional<RoomEnterArea> enteredArea(const sf::Vector2f& player_pos_px) const;

   std::vector<SubRoom>::const_iterator findSubRoom(const sf::Vector2f& p) const;
   std::vector<SubRoom>::const_iterator findSubRoom(const sf::FloatRect& p) const;
   bool correctedCamera(float& x, float& y, float focus_offset, float view_ratio_y) const;
   std::unique_ptr<ScreenTransition> makeFadeTransition();

   std::vector<SubRoom> _sub_rooms;
   std::optional<std::string> _group_name;
   int32_t _id = 0;

   //! specify how long the camera position should not be updated after entering the room
   std::optional<std::chrono::milliseconds> _camera_lock_delay{0};

   //! whenever a room is entered, a transition effect can be shown, or none when not specified
   std::optional<TransitionEffect> _transition_effect;
   float _fade_in_speed = 2.0f;
   float _fade_out_speed = 2.0f;
   std::chrono::milliseconds _delay_between_effects_ms{250};
   bool _camera_sync_after_fade_out = true;
   bool _camera_locked = false;
   
   
public:
   void deserializeEnterArea(const GameDeserializeData& data);
   
private:
   void readEntracePositions(Room::SubRoom sub_room, const GameDeserializeData& data);
};
