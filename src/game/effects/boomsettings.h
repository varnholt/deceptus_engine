#ifndef BOOMSETTINGS_H
#define BOOMSETTINGS_H

struct BoomSettings
{
   enum class ShakeType
   {
      Sine,
      Random
   };

   float _amplitude = 1.0f;
   float _boom_duration_s = 1.0f;
   ShakeType _shake_type = ShakeType::Random;
};

#endif  // BOOMSETTINGS_H
