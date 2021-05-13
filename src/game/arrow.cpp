#include "arrow.h"

#include "framework/tools/globalclock.h"
#include "texturepool.h"


const auto sprite_width = PIXELS_PER_TILE;
const auto sprite_height = PIXELS_PER_TILE;
const auto sprite_count = 5;
const auto sprites_per_row = 15;
const auto sprite_frame_time_s = 0.075f;
const auto sprite_start_frame = 10;

static constexpr auto default_bow_projectile_identifier = "data/weapons/arrow.png";

bool Arrow::_animation_initialised = false;


Arrow::Arrow()
{
   _identifier = default_bow_projectile_identifier;
   _rotating = true;
   _sticky = true;
   _weapon_type = WeaponType::Bow;

   if (!_animation_initialised)
   {
      _animation_initialised = true;

      ProjectileHitAnimation::addReferenceAnimation(
         default_bow_projectile_identifier,
         sprite_width,
         sprite_height,
         std::chrono::duration<float, std::chrono::seconds::period>{sprite_frame_time_s},
         sprite_count,
         sprites_per_row,
         sprite_start_frame
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


