#ifndef GAMEMECHANISMAUDIO_H
#define GAMEMECHANISMAUDIO_H

namespace GameMechanismAudio
{
enum class Effect
{
   BouncerJump,
   CollapsingPlatformCrumble,
   LeverOn,
   LeverOff,
};

void initialize();
void play(Effect effect);
void stop(Effect effect);
};  // namespace GameMechanismAudio

#endif // GAMEMECHANISMAUDIO_H
