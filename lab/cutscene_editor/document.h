#pragma once

#include <cstdint>
#include <string>
#include <vector>

// ---- enumerations -----------------------------------------------------------

enum class TriggerType : int32_t
{
   At = 0,   //!< fires at a wall-clock time (seconds since cutscene start)
   On = 1,   //!< fires when a named event is notified
};

enum class ActionType : int32_t
{
   SetCameraPosition   = 0,
   MoveCamera          = 1,
   SetZoom             = 2,
   PlayMusic           = 3,
   PlaySound           = 4,
   CreateSprite        = 5,
   DestroySprite       = 6,
   SetSpriteAnimation  = 7,
   SetSpriteVisible    = 8,
   MoveSprite          = 9,
   FadeIn              = 10,
   FadeOut             = 11,
   ShowDialogue        = 12,
   SetPlayerVisible    = 13,
   SetInfoLayerVisible = 14,
   UnlockCamera        = 15,
   NextLevel           = 16,
};

constexpr int32_t action_type_count = 17;

const char* actionTypeToString(ActionType type);
ActionType  stringToActionType(const std::string& str);

// ---- data model -------------------------------------------------------------

struct CutsceneEntry
{
   // trigger
   TriggerType _trigger_type = TriggerType::At;
   float       _at_time      = 0.0f;   //!< used when trigger_type == At
   std::string _on_event;              //!< used when trigger_type == On
   float       _delay        = 0.0f;   //!< optional delay before action fires (On only)

   // action
   ActionType  _action_type  = ActionType::SetCameraPosition;

   // position / movement target
   float       _x            = 0.0f;
   float       _y            = 0.0f;

   // sprite
   std::string _name;
   std::string _animation;
   std::string _animation_file = "data/sprites/animations.json";
   bool        _looped          = false;

   // visibility
   bool        _visible         = true;

   // movement / speed
   float       _speed           = 80.0f;
   std::string _event;          //!< optional completion event name

   // camera / zoom
   float       _factor          = 1.0f;
   float       _duration_s      = 1.0f;
   std::string _easing          = "linear";

   // music
   std::string _file;
   std::string _transition      = "crossfade";
   int32_t     _duration_ms     = 1000;
   std::string _post_action     = "none";

   // dialogue / sound id
   std::string _id;
};

// ---- document ---------------------------------------------------------------

class Document
{
public:
   bool load(const std::string& path);
   bool save(const std::string& path) const;
   void reset();

   std::vector<CutsceneEntry> _entries;
   std::string                _filepath;
   bool                       _modified = false;
};
