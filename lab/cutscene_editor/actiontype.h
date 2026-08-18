#pragma once

#include <cstdint>
#include <string>

/// \brief identifies the engine command a cutscene entry invokes.
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

/// \brief total number of ActionType values; must match the enum definition above.
constexpr int32_t action_type_count = 17;

/// \brief converts an ActionType to its JSON action string.
/// \param type the action type to convert.
/// \return null-terminated string, e.g. "set_camera_position".
const char* actionTypeToString(ActionType type);

/// \brief converts a JSON action string to an ActionType.
/// \param str the action string, e.g. "move_camera".
/// \return matching ActionType, or SetCameraPosition if unrecognised.
ActionType stringToActionType(const std::string& str);

/// \brief returns a pointer to an array of all action type strings in ActionType enum order.
///        the array has exactly action_type_count elements; suitable for ImGui::Combo.
/// \return pointer to a static const char* array of length action_type_count.
const char* const* actionTypeNames();
