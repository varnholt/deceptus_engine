#ifndef GAMEMECHANISMAUDIO_H
#define GAMEMECHANISMAUDIO_H


class GameMechanismAudio
{
public:
   enum class Effect
   {
      BouncerJump
   };

   void initialize();
   void play(Effect effect);

   static GameMechanismAudio& getInstance();

private:
   GameMechanismAudio() = default;
};

#endif // GAMEMECHANISMAUDIO_H
