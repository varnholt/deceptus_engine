#ifndef MUSICPLAYERTYPES_H
#define MUSICPLAYERTYPES_H

namespace MusicPlayerTypes
{
enum class TransitionType
{
   LetCurrentFinish,
   Crossfade,
   ImmediateSwitch,
   FadeOutThenNew
};

enum class PostPlaybackAction
{
   None,      // do nothing when the track ends
   Loop,      // restart the same track
   PlayNext,  // play next track in a list
};

enum class MusicTransitionState
{
   None,
   Crossfading,
   FadingOut,
};

}  // namespace MusicPlayerTypes

#endif  // MUSICPLAYERTYPES_H
