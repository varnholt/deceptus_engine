#pragma once

#include <chrono>
#include <optional>
#include <map>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

#include "framework/tmxparser/tmxobject.h"

struct Room : std::enable_shared_from_this<Room>
{
   std::shared_ptr<Room> getptr() {
       return shared_from_this();
   }

   enum class TransitionEffect
   {
      FadeOutFadeIn
   };

   Room(const sf::FloatRect& rect);

   static void deserialize(TmxObject* tmxObject, std::vector<std::shared_ptr<Room>>& rooms);
   static std::shared_ptr<Room> find(const sf::Vector2f& p, const std::vector<std::shared_ptr<Room>>& rooms);

   void startTransition();
   void lockCamera();

   std::vector<sf::FloatRect>::const_iterator findRect(const sf::Vector2f& p) const;
   bool correctedCamera(float& x, float& y, float focusOffset, float viewRatioY) const;
   // std::optional<Room> computeCurrentRoom(const sf::Vector2f& cameraCenter, const std::vector<Room>& rooms) const;

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
};

