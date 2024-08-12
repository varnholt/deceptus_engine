#ifndef INGAMEMENUAUDIO_H
#define INGAMEMENUAUDIO_H

class InGameMenuAudio
{
public:
   InGameMenuAudio();

   enum class SoundEffect
   {
      MenuNext,
      MenuOpen,
      MenuClose,
      ItemSelect,
      ItemNavigate,
   };

   void play(SoundEffect effect);
};

#endif  // INGAMEMENUAUDIO_H
