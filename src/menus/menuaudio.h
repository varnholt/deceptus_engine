#ifndef MENUAUDIO_H
#define MENUAUDIO_H

namespace MenuAudio
{
enum class SoundEffect
{
   Apply,
   MenuBack,
   ItemTick,
   ItemNavigate,
   ItemSelect,
};

void initialize();
void play(SoundEffect effect);
};  // namespace MenuAudio

#endif // MENUAUDIO_H
