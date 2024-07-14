#ifndef GAMEAUDIO_H
#define GAMEAUDIO_H


class GameAudio
{
public:
   enum class SoundEffect
   {
      GameStatePause,
      GameStateResume,
   };

   void initialize();
   void play(SoundEffect effect);

   static GameAudio& getInstance();

private:
   GameAudio() = default;
};

#endif // GAMEAUDIO_H
