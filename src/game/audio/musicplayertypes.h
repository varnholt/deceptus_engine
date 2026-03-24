#ifndef MUSICPLAYERTYPES_H
#define MUSICPLAYERTYPES_H

namespace MusicPlayerTypes
{
/// \brief defines how a new music track should replace the currently playing track.
enum class TransitionType
{
   LetCurrentFinish,
   Crossfade,
   ImmediateSwitch,
   FadeOutThenNew
};

/// \brief defines what the music player does after a track finishes playback.
enum class PostPlaybackAction
{
   None,      // do nothing when the track ends
   Loop,      // restart the same track
   PlayNext,  // play next track in a list
};

/// \brief tracks the internal state of an ongoing music transition.
enum class MusicTransitionState
{
   None,
   Crossfading,
   FadingOut,
};

}  // namespace MusicPlayerTypes

#endif  // MUSICPLAYERTYPES_H
