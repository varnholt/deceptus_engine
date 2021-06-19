#include "detonationanimation.h"

#include <ctime>
#include <iostream>
#include <math.h>

#include "texturepool.h"

// big detonations are: 6 x 6 x 24
//    8 rows
//    10 columns
//    frames per animation: 16
// small detonations are: 3 x 3 x 24
//    4 rows
//    16 columns
//    frames per animation: 16

AnimationFrameData& getFrameData(DetonationAnimation::DetonationType type)
{
   static std::vector<AnimationFrameData> _frame_data_small;
   static std::vector<AnimationFrameData> _frame_data_big;

   // init frame data
   if (_frame_data_small.empty())
   {
      std::vector<sf::Time> frame_times;
      for (auto i = 0u; i < 16; i++)
      {
         frame_times.push_back(sf::seconds(0.075f));
      }

      // big detonations
      for (auto i = 0u; i < 4; i++)
      {
         AnimationFrameData fd(
            TexturePool::getInstance().get("data/sprites/explosions.png"),
            sf::Vector2f(3 * 24, 3 * 24),
            6 * 24,
            6 * 24,
            16,
            10,
            frame_times,
            i * 20
         );

         _frame_data_small.push_back(fd);
      }

      // small detonations
      for (auto i = 0u; i < 4; i++)
      {
         AnimationFrameData fd(
            TexturePool::getInstance().get("data/sprites/explosions.png"),
            sf::Vector2f(3 * 12, 3 * 12),
            3 * 24,
            3 * 24,
            16,
            32,
            frame_times,
            16 * 32 + i * 32 // we wanna skip the first 16 rows since those are the big detonations
         );

         _frame_data_big.push_back(fd);
      }
   }

   switch (type)
   {
      case DetonationAnimation::DetonationType::Big:
         return _frame_data_small[std::rand() % _frame_data_big.size()];
         break;
      case DetonationAnimation::DetonationType::Small:
         return _frame_data_small[std::rand() % _frame_data_small.size()];
         break;
   }

   // this is an error
   return _frame_data_small[0];
}


DetonationAnimation::DetonationAnimation(
   const std::vector<DetonationAnimation::DetonationRing>& rings
)
{
   // compute all positions
   // have one detonation per position

   std::srand(static_cast<int32_t>(std::time(nullptr)));

   auto ring_index = 0u;
   for (auto& ring : rings)
   {
      auto detonation_type = (ring_index == 0) ? DetonationType::Big : DetonationType::Small;

      float angle = 0.0f;
      float angle_increment = static_cast<float>(M_PI) / static_cast<float>(ring._detonation_count);

      for (auto i = 0; i < ring._detonation_count; i++)
      {
         const auto rand_x_normalized = (2.0f * std::rand() / static_cast<float>(RAND_MAX)) - 1.0f;
         const auto rand_y_normalized = (2.0f * std::rand() / static_cast<float>(RAND_MAX)) - 1.0f;

         const auto x = ring._center.x + ring._variance_position.x * rand_x_normalized + (cos(angle) * ring._radius);
         const auto y = ring._center.x + ring._variance_position.y * rand_y_normalized + (sin(angle) * ring._radius);

         angle += angle_increment;

         // std::cout << x << std::endl;
         // std::cout << y << std::endl;

         auto& frame_data = getFrameData(detonation_type);

         // bend the play time a bit so they don't all end at exactly the same time
         const auto time_stretch_factor = (2.0f * (std::rand() / static_cast<float>(RAND_MAX)) - 1.0f) * ring._variance_animation_speed;
         for (auto& frame_time : frame_data._frame_times)
         {
            frame_time += sf::seconds(time_stretch_factor);
         }

         Animation animation;
         animation.setPosition(x, y);
         animation._frames = frame_data._frames;
         animation._color_texture = frame_data._texture;
         animation.setFrameTimes(frame_data._frame_times);
         animation.setOrigin(frame_data._origin);
         animation._reset_to_first_frame = false;
         animation.updateVertices();
         animation.play();

         // std::cout << "setting animation rotation to " << angle << std::endl;

         _animations.push_back(animation);
      }

      ring_index++;
   }
}


DetonationAnimation DetonationAnimation::makeHugeExplosion(const sf::Vector2f center)
{
   DetonationRing ring_a;
   DetonationRing ring_b;
   DetonationRing ring_c;

   ring_a._center = center;
   ring_a._radius = 0;
   ring_a._detonation_count = 1;

   ring_b._center = center;
   ring_b._radius = 2;
   ring_b._detonation_count = 4;
   ring_b._variance_position = sf::Vector2f(0.1f, 0.1f);
   ring_b._variance_animation_speed = 0.005f;

   ring_c._center = center;
   ring_c._radius = 5;
   ring_c._detonation_count = 9;
   ring_c._variance_position = sf::Vector2f(0.3f, 0.3f);
   ring_c._variance_animation_speed = 0.005f;

   std::vector<DetonationRing> rings;

   rings.push_back(ring_a);
   rings.push_back(ring_b);
   rings.push_back(ring_c);

   DetonationAnimation animation(rings);
   return animation;
}


const std::vector<Animation>& DetonationAnimation::getAnimations() const
{
   return _animations;
}


