#ifndef GAMEAUDIO_H
#define GAMEAUDIO_H

/// \brief singleton wrapper for game-state UI sound effects.
class GameAudio
{
public:
   /// \brief identifiers for supported game audio samples.
   enum class SoundEffect
   {
      GameStatePause,
      GameStateResume,
   };

   /// \brief registers pause and resume samples with the audio system.
   void initialize();

   /// \brief plays one of the registered game-state sound effects.
   /// \param effect sound effect identifier to trigger.
   void play(SoundEffect effect);

   /// \brief gets the global game audio singleton.
   /// \return shared process-wide GameAudio instance.
   static GameAudio& getInstance();

private:
   /// \brief constructs the game audio singleton.
   GameAudio() = default;
};

#endif  // GAMEAUDIO_H
