#ifndef INGAMEMENUAUDIO_H
#define INGAMEMENUAUDIO_H

/// \brief preloads and plays sound samples used by the in-game menu ui.
class InGameMenuAudio
{
public:
   /// \brief registers all in-game menu sound samples in the global audio system.
   InGameMenuAudio();

   /// \brief identifies one of the menu sound effects mapped to wav samples.
   enum class SoundEffect
   {
      MenuNext,
      MenuOpen,
      MenuClose,
      ItemSelect,
      ItemNavigate,
   };

   /// \brief plays the sample corresponding to the requested menu sound effect.
   /// \param effect sound effect enum value to play.
   void play(SoundEffect effect);
};

#endif  // INGAMEMENUAUDIO_H
