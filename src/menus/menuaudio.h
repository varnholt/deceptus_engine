#ifndef MENUAUDIO_H
#define MENUAUDIO_H

namespace MenuAudio
{
   /// \brief identifies menu sound samples used for navigation and confirmation feedback.
   enum class SoundEffect
   {
      Apply,
      ItemTick,
      ItemNavigate,
      ItemSelect,
      MenuBack,
   };

   /// \brief preloads menu sound samples into the global audio system.
   void initialize();

   /// \brief plays the sample associated with a menu interaction.
   /// \param effect sound effect id mapped to a specific sample file.
   void play(SoundEffect effect);
};  // namespace MenuAudio

#endif  // MENUAUDIO_H
