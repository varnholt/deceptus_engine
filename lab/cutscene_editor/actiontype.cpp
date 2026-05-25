#include "actiontype.h"

// order must match the ActionType enum exactly; all three public functions index
// into this table so the enum-to-string mapping is maintained in one place.
static const char* action_name_table[action_type_count] = {
   "set_camera_position",    // 0  ActionType::SetCameraPosition
   "move_camera",            // 1  ActionType::MoveCamera
   "set_zoom",               // 2  ActionType::SetZoom
   "play_music",             // 3  ActionType::PlayMusic
   "play_sound",             // 4  ActionType::PlaySound
   "create_sprite",          // 5  ActionType::CreateSprite
   "destroy_sprite",         // 6  ActionType::DestroySprite
   "set_sprite_animation",   // 7  ActionType::SetSpriteAnimation
   "set_sprite_visible",     // 8  ActionType::SetSpriteVisible
   "move_sprite",            // 9  ActionType::MoveSprite
   "fade_in",                // 10 ActionType::FadeIn
   "fade_out",               // 11 ActionType::FadeOut
   "show_dialogue",          // 12 ActionType::ShowDialogue
   "set_player_visible",     // 13 ActionType::SetPlayerVisible
   "set_info_layer_visible", // 14 ActionType::SetInfoLayerVisible
   "unlock_camera",          // 15 ActionType::UnlockCamera
   "next_level",             // 16 ActionType::NextLevel
};

const char* actionTypeToString(ActionType type)
{
   const auto index = static_cast<int32_t>(type);
   if (index < 0 || index >= action_type_count)
   {
      return "";
   }
   return action_name_table[index];
}

ActionType stringToActionType(const std::string& str)
{
   for (int32_t index = 0; index < action_type_count; index++)
   {
      if (str == action_name_table[index])
      {
         return static_cast<ActionType>(index);
      }
   }
   return ActionType::SetCameraPosition;
}

const char* const* actionTypeNames()
{
   return action_name_table;
}
