#ifndef BOOMSETTINGS_H
#define BOOMSETTINGS_H

/// \brief stores parameters that control camera shake strength, duration, and waveform.
struct BoomSettings
{
   /// \brief selects the envelope algorithm used to produce shake offsets.
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
