#include "arrow.h"

#include "framework/tools/globalclock.h"
#include "texturepool.h"


const auto sprite_width = PIXELS_PER_TILE;
const auto sprite_height = PIXELS_PER_TILE;
const auto sprite_count = 5;
const auto sprites_per_row = 15;
const auto sprite_frame_time = 0.075f;
const auto sprite_start_frame = 10;


bool Arrow::_animation_initialised = false;


Arrow::Arrow()
{
   _rotating = true;
   _sticky = true;
   _weapon_type = WeaponType::Bow;

   if (!_animation_initialised)
   {
      _animation_initialised = true;

      auto texture = TexturePool::getInstance().get("data/weapons/arrow.png");

      std::vector<sf::Time> frame_times;
      for (auto i = 0u; i < sprite_count; i++)
      {
         frame_times.push_back(sf::seconds(sprite_frame_time));
      }

      sf::Vector2f origin(
         static_cast<float_t>(PIXELS_PER_TILE / 2),
         static_cast<float_t>(PIXELS_PER_TILE / 2)
      );

      _hit_animations.emplace(
         _weapon_type,
         AnimationFrameData{
            texture,
            origin,
            sprite_width,
            sprite_height,
            sprite_count,
            sprites_per_row,
            frame_times,
            sprite_start_frame
         }
      );
   }
}


void Arrow::updateTextureRect()
{
   const auto now = GlobalClock::getInstance()->getElapsedTimeInMs();

   auto frame = 0;
   const auto dt = now - _start_time;

   if (dt < 300)
   {
      frame = 0;
   }
   else if (dt < 500)
   {
      frame = 1;
   }
   else
   {
      frame = 2;
   }

   sf::Rect<int32_t> texture_rect;

   texture_rect.top    = 1 * PIXELS_PER_TILE;
   texture_rect.left   = frame * PIXELS_PER_TILE;
   texture_rect.width  = PIXELS_PER_TILE;
   texture_rect.height = PIXELS_PER_TILE;

   _sprite.setTextureRect(texture_rect);
}


