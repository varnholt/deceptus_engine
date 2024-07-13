#include "arrow.h"

#include <iostream>

#include "framework/tools/globalclock.h"
#include "game/weapons/projectilehitaudio.h"
#include "texturepool.h"


const auto sprite_width = PIXELS_PER_TILE;
const auto sprite_height = PIXELS_PER_TILE;
const auto sprite_count = 10;
const auto sprites_per_row = 15;
const auto sprite_frame_time_s = 0.075f;
const auto sprite_start_frame = 20;

constexpr auto default_bow_projectile_identifier = "data/weapons/arrow.png";

bool Arrow::_animation_initialised = false;

namespace
{
int32_t created_count = 0;
int32_t deleted_count = 0;
}  // namespace

Arrow::Arrow()
{
   _projectile_identifier = default_bow_projectile_identifier;
   _rotating = true;
   _sticky = true;
   _weapon_type = WeaponType::Bow;

   if (!_animation_initialised)
   {
      _animation_initialised = true;

      ProjectileHitAudio::addReferenceSamples(
         default_bow_projectile_identifier,
         {ProjectileHitAudio::ProjectileHitSample{"arrow_hit_1.wav", 0.3f},
          ProjectileHitAudio::ProjectileHitSample{"arrow_hit_2.wav", 0.3f}}
      );

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

   created_count++;
}

Arrow::~Arrow()
{
   deleted_count++;

   // std::cout << "created: " << created_count << " deleted: " << deleted_count << std::endl;
}
