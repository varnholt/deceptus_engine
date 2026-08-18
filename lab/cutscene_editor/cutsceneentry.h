#pragma once

#include "actiontype.h"
#include "triggertype.h"

#include <cstdint>
#include <string>

/// \brief one row in a cutscene JSON file — a trigger condition and the action it dispatches.
///        all possible action fields are stored in a single flat struct; only the fields
///        relevant to _action_type are written to JSON on save.
struct CutsceneEntry
{
   // -- trigger --
   TriggerType _trigger_type = TriggerType::At;    //!< whether this entry fires on a time or on an event
   float       _at_time      = 0.0f;               //!< seconds since cutscene start (TriggerType::At only)
   std::string _on_event;                          //!< event name to listen for (TriggerType::On only)
   float       _delay        = 0.0f;               //!< additional seconds to wait after the event fires

   // -- action --
   ActionType  _action_type  = ActionType::SetCameraPosition;  //!< which engine command to invoke

   // -- position / target --
   float       _x            = 0.0f;               //!< horizontal coordinate or target x
   float       _y            = 0.0f;               //!< vertical coordinate or target y

   // -- sprite --
   std::string _name;                              //!< sprite instance name
   std::string _animation;                         //!< animation key within the animation file
   std::string _animation_file = "data/sprites/animations.json";  //!< path to the animation JSON
   bool        _looped         = false;            //!< whether the animation repeats

   // -- visibility --
   bool        _visible        = true;             //!< target visible state

   // -- movement --
   float       _speed          = 80.0f;            //!< movement speed in pixels per second
   std::string _event;                             //!< event emitted when the move completes (optional)

   // -- camera / zoom --
   float       _factor         = 1.0f;             //!< zoom scale factor
   float       _duration_s     = 1.0f;             //!< transition duration in seconds
   std::string _easing         = "linear";         //!< easing curve: linear, ease_in, ease_out, ease_in_out

   // -- music --
   std::string _file;                              //!< path to the music file
   std::string _transition     = "crossfade";      //!< music transition mode
   int32_t     _duration_ms    = 1000;             //!< transition duration in milliseconds
   std::string _post_action    = "none";           //!< what happens after playback ends

   // -- dialogue / sound --
   std::string _id;                               //!< dialogue id or sound id
};
