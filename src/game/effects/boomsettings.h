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

   /// share of the duration, at its end, over which the random shake dies down. 1.0 fades it over the whole
   /// duration, smaller values hold it at full strength first, which is what a quake needs
   float _fade_out_share = 1.0f;
};

#endif  // BOOMSETTINGS_H
