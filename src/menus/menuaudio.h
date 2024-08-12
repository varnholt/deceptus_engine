#ifndef MENUAUDIO_H
#define MENUAUDIO_H

namespace MenuAudio
{
   enum class SoundEffect
   {
      Apply,
      ItemTick,
      ItemNavigate,
      ItemSelect,
      MenuBack,
   };

   void initialize();
   void play(SoundEffect effect);
};  // namespace MenuAudio

#endif  // MENUAUDIO_H
