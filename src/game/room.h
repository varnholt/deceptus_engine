#pragma once

#include <chrono>
#include <optional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

#include "framework/tmxparser/tmxobject.h"
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
   std::shared_ptr<Room> getptr() {
       return shared_from_this();
   }

   enum class TransitionEffect
   {
      FadeOutFadeIn
   };


   enum class EnteredDirection : char
   {
      Invalid  = '?',
      Left     = 'l',
      Right    = 'r',
      Top      = 't',
      Bottom   = 'b'
   };

   Room(GameNode* parent, const sf::FloatRect& rect);

   static void deserialize(GameNode* parent, TmxObject* tmx_object, std::vector<std::shared_ptr<Room>>& rooms);
   static std::shared_ptr<Room> find(const sf::Vector2f& p, const std::vector<std::shared_ptr<Room>>& rooms);

   void startTransition();
   void lockCamera();
   void movePlayerToRoomStartPosition();
   void syncCamera();

   EnteredDirection enteredDirection(const sf::Vector2f& player_pos_px) const;
   std::optional<sf::FloatRect> activeRect(const sf::Vector2f& player_pos_px) const;

   std::vector<sf::FloatRect>::const_iterator findRect(const sf::Vector2f& p) const;
   bool correctedCamera(float& x, float& y, float focus_offset, float view_ratio_y) const;
   std::unique_ptr<ScreenTransition> makeFadeTransition();

   std::vector<sf::FloatRect> _rects;
   std::string _name;
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

   //! start positions when room was entered
   std::optional<sf::Vector2i> _start_position_l;
   std::optional<sf::Vector2i> _start_position_r;
   std::optional<sf::Vector2i> _start_position_t;
   std::optional<sf::Vector2i> _start_position_b;
   std::optional<sf::Vector2i> _start_offset_l;
   std::optional<sf::Vector2i> _start_offset_r;
};

