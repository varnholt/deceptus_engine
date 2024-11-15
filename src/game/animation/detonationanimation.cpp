#include "detonationanimation.h"

#include <math.h>
#include <algorithm>
#include <cassert>
#include <ctime>
#include <numbers>

#include "game/io/texturepool.h"

// big detonations are: 6 x 6 x 24
//    8 rows
//    10 columns
//    frames per animation: 16
// small detonations are: 3 x 3 x 24
//    4 rows
//    16 columns
//    frames per animation: 16

AnimationFrameData& DetonationAnimation::getFrameData(DetonationAnimation::DetonationType type)
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

         // prepend empty frame for time offsetting
         fd._frame_times.insert(fd._frame_times.begin(), sf::seconds(0.0f));
         fd._frames.insert(fd._frames.begin(), {});

         _frame_data_big.push_back(fd);
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
            16 * 32 + i * 32  // we wanna skip the first 16 rows since those are the big detonations
         );

         // prepend empty frame for time offsetting
         fd._frame_times.insert(fd._frame_times.begin(), sf::seconds(0.0f));
         fd._frames.insert(fd._frames.begin(), {});

         _frame_data_small.push_back(fd);
      }
   }

   switch (type)
   {
      case DetonationAnimation::DetonationType::Big:
         return _frame_data_big[std::rand() % _frame_data_big.size()];
         break;
      case DetonationAnimation::DetonationType::Small:
         return _frame_data_small[std::rand() % _frame_data_small.size()];
         break;
   }

   // this is an error
   assert(false);
   return _frame_data_small[0];
}

DetonationAnimation::DetonationAnimation(const std::vector<DetonationAnimation::DetonationRing>& rings)
{
   // compute all positions
   // have one detonation per position

   std::srand(static_cast<int32_t>(std::time(nullptr)));

   auto ring_index = 0u;
   for (auto& ring : rings)
   {
      auto detonation_type = (ring_index == 0) ? DetonationType::Big : DetonationType::Small;

      auto angle = 0.0f;
      const auto angle_increment = static_cast<float>(2.0f * std::numbers::pi_v<float>) / static_cast<float>(ring._detonation_count);

      for (auto i = 0; i < ring._detonation_count; i++)
      {
         const auto rand_x_normalized = (2.0f * std::rand() / static_cast<float>(RAND_MAX)) - 1.0f;
         const auto rand_y_normalized = (2.0f * std::rand() / static_cast<float>(RAND_MAX)) - 1.0f;

         const auto x = ring._center.x + (cos(angle) * ring._radius) + ring._variance_position.x * rand_x_normalized;
         const auto y = ring._center.y + (sin(angle) * ring._radius) + ring._variance_position.y * rand_y_normalized;

         angle += angle_increment;

         auto& frame_data = getFrameData(detonation_type);

         // bend the play time a bit so they don't all end at exactly the same time
         const auto rand_normalized = std::rand() / static_cast<float>(RAND_MAX);
         const auto time_stretch_factor = (2.0f * rand_normalized - 1.0f) * ring._variance_animation_speed;

         std::transform(
            frame_data._frame_times.begin(),
            frame_data._frame_times.end(),
            frame_data._frame_times.begin(),
            [time_stretch_factor](const auto& time) { return time + sf::seconds(time_stretch_factor); }
         );

         // add offset, the further out, the bigger, 0 in the middle
         frame_data._frame_times[0] = sf::seconds(ring_index * rand_normalized * ring._variance_animation_speed);

         auto animation = std::make_shared<Animation>();
         animation->setPosition(x, y);
         animation->_frames = frame_data._frames;
         animation->_color_texture = frame_data._texture;
         animation->setFrameTimes(frame_data._frame_times);
         animation->setOrigin(frame_data._origin);
         animation->_reset_to_first_frame = false;
         animation->updateVertices();
         animation->play();

         // Log::Info() << "setting animation rotation to " << angle;

         _animations.push_back(std::move(animation));
      }

      ring_index++;
   }
}

DetonationAnimation DetonationAnimation::makeHugeExplosion(const sf::Vector2f& center)
{
   DetonationRing ring_a;
   DetonationRing ring_b;
   DetonationRing ring_c;

   ring_a._center = center;
   ring_a._radius = 0;
   ring_a._detonation_count = 1;

   ring_b._center = center;
   ring_b._radius = 35;
   ring_b._detonation_count = 4;
   ring_b._variance_position = sf::Vector2f(10.0f, 10.0f);
   ring_b._variance_animation_speed = 0.005f;

   ring_c._center = center;
   ring_c._radius = 75;
   ring_c._detonation_count = 9;
   ring_c._variance_position = sf::Vector2f(15.0f, 30.0f);
   ring_c._variance_animation_speed = 0.005f;

   DetonationAnimation detonation({ring_a, ring_b, ring_c});
   return detonation;
}

const std::vector<std::shared_ptr<Animation>>& DetonationAnimation::getAnimations() const
{
   return _animations;
}
