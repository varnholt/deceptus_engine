#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>

#include "editor.h"

#include <imgui.h>

#include <cstdio>
#include <cstring>
#include <vector>

// ---- file dialog helpers ----------------------------------------------------

static std::string openFileDialog()
{
   OPENFILENAMEA ofn   = {};
   char          filename_buffer[MAX_PATH] = {};
   ofn.lStructSize     = sizeof(ofn);
   ofn.lpstrFilter     = "JSON Files\0*.json\0All Files\0*.*\0";
   ofn.lpstrFile       = filename_buffer;
   ofn.nMaxFile        = MAX_PATH;
   ofn.Flags           = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
   if (GetOpenFileNameA(&ofn))
   {
      return filename_buffer;
   }
   return {};
}

static std::string saveFileDialog(const std::string& current_path)
{
   OPENFILENAMEA ofn   = {};
   char          filename_buffer[MAX_PATH] = {};
   if (!current_path.empty())
   {
      std::strncpy(filename_buffer, current_path.c_str(), MAX_PATH - 1);
   }
   ofn.lStructSize     = sizeof(ofn);
   ofn.lpstrFilter     = "JSON Files\0*.json\0All Files\0*.*\0";
   ofn.lpstrFile       = filename_buffer;
   ofn.nMaxFile        = MAX_PATH;
   ofn.lpstrDefExt     = "json";
   ofn.Flags           = OFN_OVERWRITEPROMPT;
   if (GetSaveFileNameA(&ofn))
   {
      return filename_buffer;
   }
   return {};
}

// ---- action name table (index must match ActionType enum values) -------------

static const char* action_names[action_type_count] = {
   "set_camera_position",   // 0
   "move_camera",           // 1
   "set_zoom",              // 2
   "play_music",            // 3
   "play_sound",            // 4
   "create_sprite",         // 5
   "destroy_sprite",        // 6
   "set_sprite_animation",  // 7
   "set_sprite_visible",    // 8
   "move_sprite",           // 9
   "fade_in",               // 10
   "fade_out",              // 11
   "show_dialogue",         // 12
   "set_player_visible",    // 13
   "set_info_layer_visible",// 14
   "unlock_camera",         // 15
   "next_level",            // 16
};

// ---- helper: find the index of a string in a const char* array --------------

static int32_t findStringIndex(const char* const* options, int32_t option_count, const std::string& value)
{
   for (int32_t option_index = 0; option_index < option_count; option_index++)
   {
      if (value == options[option_index])
      {
         return option_index;
      }
   }
   return 0;
}

// ---- Editor -----------------------------------------------------------------

bool Editor::inputText(const char* label, std::string& value, size_t max_length)
{
   std::vector<char> text_buffer(max_length);
   std::strncpy(text_buffer.data(), value.c_str(), max_length - 1);
   if (ImGui::InputText(label, text_buffer.data(), max_length))
   {
      value = text_buffer.data();
      return true;
   }
   return false;
}

void Editor::openFile()
{
   const auto selected_path = openFileDialog();
   if (!selected_path.empty())
   {
      _document.load(selected_path);
      _selected_entry_index = -1;
   }
}

void Editor::saveFile()
{
   if (_document._filepath.empty())
   {
      saveFileAs();
      return;
   }
   if (_document.save(_document._filepath))
   {
      _document._modified = false;
   }
}

void Editor::saveFileAs()
{
   const auto selected_path = saveFileDialog(_document._filepath);
   if (!selected_path.empty())
   {
      _document._filepath = selected_path;
      if (_document.save(_document._filepath))
      {
         _document._modified = false;
      }
   }
}

void Editor::draw()
{
   // keyboard shortcuts
   const ImGuiIO& io = ImGui::GetIO();
   if (io.KeyCtrl && !io.WantTextInput)
   {
      if (ImGui::IsKeyPressed(ImGuiKey_N, false))
      {
         _document.reset();
         _selected_entry_index = -1;
      }
      else if (ImGui::IsKeyPressed(ImGuiKey_O, false))
      {
         openFile();
      }
      else if (ImGui::IsKeyPressed(ImGuiKey_S, false))
      {
         if (io.KeyShift)
         {
            saveFileAs();
         }
         else
         {
            saveFile();
         }
      }
   }

   ImGui::SetNextWindowPos({0.0f, 0.0f});
   ImGui::SetNextWindowSize(io.DisplaySize);
   ImGui::Begin(
      "##main",
      nullptr,
      ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoResize   |
      ImGuiWindowFlags_NoMove     |
      ImGuiWindowFlags_MenuBar    |
      ImGuiWindowFlags_NoBringToFrontOnFocus
   );

   drawMenuBar();

   const float list_panel_width = ImGui::GetContentRegionAvail().x * 0.35f;

   ImGui::BeginChild("##entry_list", {list_panel_width, 0.0f}, true);
   drawEntryList();
   ImGui::EndChild();

   ImGui::SameLine();

   ImGui::BeginChild("##entry_editor", {0.0f, 0.0f}, true);
   drawEntryEditor();
   ImGui::EndChild();

   ImGui::End();
}

void Editor::drawMenuBar()
{
   if (!ImGui::BeginMenuBar())
   {
      return;
   }

   if (ImGui::BeginMenu("File"))
   {
      if (ImGui::MenuItem("New", "Ctrl+N"))
      {
         _document.reset();
         _selected_entry_index = -1;
      }
      if (ImGui::MenuItem("Open...", "Ctrl+O"))
      {
         openFile();
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Save", "Ctrl+S", false, !_document._filepath.empty()))
      {
         saveFile();
      }
      if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
      {
         saveFileAs();
      }
      ImGui::EndMenu();
   }

   // show current file path and modified marker in the menu bar
   ImGui::Separator();
   if (_document._filepath.empty())
   {
      ImGui::TextDisabled("[unsaved]");
   }
   else
   {
      const auto display_path = _document._filepath + (_document._modified ? " *" : "");
      ImGui::TextDisabled("%s", display_path.c_str());
   }

   ImGui::EndMenuBar();
}

void Editor::drawEntryList()
{
   ImGui::TextUnformatted("Entries");
   ImGui::Separator();

   const float buttons_height = ImGui::GetFrameHeightWithSpacing() + 4.0f;
   ImGui::BeginChild("##list_scroll", {0.0f, ImGui::GetContentRegionAvail().y - buttons_height}, false);

   for (int32_t entry_index = 0; entry_index < static_cast<int32_t>(_document._entries.size()); entry_index++)
   {
      const auto& entry = _document._entries[entry_index];

      char trigger_label[128];
      if (entry._trigger_type == TriggerType::At)
      {
         std::snprintf(trigger_label, sizeof(trigger_label), "at=%.2f", entry._at_time);
      }
      else if (entry._delay > 0.0f)
      {
         std::snprintf(trigger_label, sizeof(trigger_label), "on=%-20s +%.1fs", entry._on_event.c_str(), entry._delay);
      }
      else
      {
         std::snprintf(trigger_label, sizeof(trigger_label), "on=%-24s", entry._on_event.c_str());
      }

      char selectable_label[256];
      std::snprintf(
         selectable_label,
         sizeof(selectable_label),
         "[%02d] %s | %s##entry_%d",
         entry_index + 1,
         trigger_label,
         actionTypeToString(entry._action_type),
         entry_index
      );

      if (ImGui::Selectable(selectable_label, _selected_entry_index == entry_index))
      {
         _selected_entry_index = entry_index;
      }
   }

   ImGui::EndChild();

   // action buttons
   if (ImGui::Button("+ Add"))
   {
      _document._entries.push_back(CutsceneEntry{});
      _selected_entry_index = static_cast<int32_t>(_document._entries.size()) - 1;
      _document._modified   = true;
   }
   ImGui::SameLine();
   const bool can_remove = _selected_entry_index >= 0 && _selected_entry_index < static_cast<int32_t>(_document._entries.size());
   if (ImGui::Button("- Remove") && can_remove)
   {
      _document._entries.erase(_document._entries.begin() + _selected_entry_index);
      _selected_entry_index = std::min(_selected_entry_index, static_cast<int32_t>(_document._entries.size()) - 1);
      _document._modified   = true;
   }
   ImGui::SameLine();
   const bool can_move_up = _selected_entry_index > 0;
   if (ImGui::Button("^") && can_move_up)
   {
      std::swap(_document._entries[_selected_entry_index], _document._entries[_selected_entry_index - 1]);
      _selected_entry_index--;
      _document._modified = true;
   }
   ImGui::SameLine();
   const bool can_move_down = _selected_entry_index >= 0 && _selected_entry_index < static_cast<int32_t>(_document._entries.size()) - 1;
   if (ImGui::Button("v") && can_move_down)
   {
      std::swap(_document._entries[_selected_entry_index], _document._entries[_selected_entry_index + 1]);
      _selected_entry_index++;
      _document._modified = true;
   }
}

void Editor::drawEntryEditor()
{
   const bool valid_selection = _selected_entry_index >= 0 &&
                                _selected_entry_index < static_cast<int32_t>(_document._entries.size());
   if (!valid_selection)
   {
      ImGui::TextDisabled("No entry selected.");
      return;
   }

   auto& entry = _document._entries[_selected_entry_index];

   // ---- trigger section ----
   ImGui::TextUnformatted("Trigger");
   ImGui::Separator();

   int32_t trigger_radio = static_cast<int32_t>(entry._trigger_type);
   if (ImGui::RadioButton("At  (time-based)", &trigger_radio, 0))
   {
      entry._trigger_type = TriggerType::At;
      _document._modified = true;
   }
   ImGui::SameLine();
   if (ImGui::RadioButton("On  (event-based)", &trigger_radio, 1))
   {
      entry._trigger_type = TriggerType::On;
      _document._modified = true;
   }

   ImGui::Spacing();

   if (entry._trigger_type == TriggerType::At)
   {
      if (ImGui::InputFloat("Time (seconds)", &entry._at_time, 0.1f, 1.0f, "%.3f"))
      {
         _document._modified = true;
      }
   }
   else
   {
      if (inputText("Event name", entry._on_event))
      {
         _document._modified = true;
      }
      if (ImGui::InputFloat("Delay (seconds)", &entry._delay, 0.1f, 1.0f, "%.3f"))
      {
         if (entry._delay < 0.0f)
         {
            entry._delay = 0.0f;
         }
         _document._modified = true;
      }
   }

   // ---- action section ----
   ImGui::Spacing();
   ImGui::TextUnformatted("Action");
   ImGui::Separator();

   int32_t action_index = static_cast<int32_t>(entry._action_type);
   if (ImGui::Combo("Action type", &action_index, action_names, action_type_count))
   {
      entry._action_type  = static_cast<ActionType>(action_index);
      _document._modified = true;
   }

   // ---- fields section ----
   ImGui::Spacing();
   ImGui::TextUnformatted("Fields");
   ImGui::Separator();

   drawActionFields(entry);
}

void Editor::drawActionFields(CutsceneEntry& entry)
{
   static const char* easing_options[]      = {"linear", "ease_in", "ease_out", "ease_in_out"};
   static const char* transition_options[]  = {"let_current_finish", "crossfade", "immediate", "fade_out_then_new"};
   static const char* post_action_options[] = {"none", "loop", "play_next"};

   switch (entry._action_type)
   {
      case ActionType::SetCameraPosition:
      {
         if (ImGui::InputFloat("X", &entry._x, 1.0f, 10.0f, "%.1f"))    { _document._modified = true; }
         if (ImGui::InputFloat("Y", &entry._y, 1.0f, 10.0f, "%.1f"))    { _document._modified = true; }
         break;
      }
      case ActionType::MoveCamera:
      {
         if (ImGui::InputFloat("Target X",           &entry._x,          1.0f,  10.0f,  "%.1f")) { _document._modified = true; }
         if (ImGui::InputFloat("Target Y",           &entry._y,          1.0f,  10.0f,  "%.1f")) { _document._modified = true; }
         if (ImGui::InputFloat("Duration (seconds)", &entry._duration_s, 0.1f,  1.0f,   "%.2f")) { _document._modified = true; }
         int32_t easing_index = findStringIndex(easing_options, 4, entry._easing);
         if (ImGui::Combo("Easing", &easing_index, easing_options, 4))
         {
            entry._easing       = easing_options[easing_index];
            _document._modified = true;
         }
         if (inputText("Event (optional)", entry._event))                { _document._modified = true; }
         break;
      }
      case ActionType::SetZoom:
      {
         if (ImGui::InputFloat("Zoom factor", &entry._factor, 0.05f, 0.25f, "%.2f")) { _document._modified = true; }
         break;
      }
      case ActionType::PlayMusic:
      {
         if (inputText("File path", entry._file, 512))                   { _document._modified = true; }
         int32_t transition_index = findStringIndex(transition_options, 4, entry._transition);
         if (ImGui::Combo("Transition", &transition_index, transition_options, 4))
         {
            entry._transition   = transition_options[transition_index];
            _document._modified = true;
         }
         if (ImGui::InputInt("Duration (ms)", &entry._duration_ms, 100, 1000)) { _document._modified = true; }
         int32_t post_action_index = findStringIndex(post_action_options, 3, entry._post_action);
         if (ImGui::Combo("Post action", &post_action_index, post_action_options, 3))
         {
            entry._post_action  = post_action_options[post_action_index];
            _document._modified = true;
         }
         break;
      }
      case ActionType::PlaySound:
      {
         if (inputText("Sound ID", entry._id))                           { _document._modified = true; }
         break;
      }
      case ActionType::CreateSprite:
      {
         if (inputText("Sprite name", entry._name))                     { _document._modified = true; }
         if (inputText("Animation file", entry._animation_file, 512))   { _document._modified = true; }
         if (inputText("Animation", entry._animation))                  { _document._modified = true; }
         if (ImGui::InputFloat("X", &entry._x, 1.0f, 10.0f, "%.1f"))   { _document._modified = true; }
         if (ImGui::InputFloat("Y", &entry._y, 1.0f, 10.0f, "%.1f"))   { _document._modified = true; }
         if (ImGui::Checkbox("Looped", &entry._looped))                 { _document._modified = true; }
         break;
      }
      case ActionType::DestroySprite:
      {
         if (inputText("Sprite name", entry._name))                     { _document._modified = true; }
         break;
      }
      case ActionType::SetSpriteAnimation:
      {
         if (inputText("Sprite name", entry._name))                     { _document._modified = true; }
         if (inputText("Animation", entry._animation))                  { _document._modified = true; }
         if (ImGui::Checkbox("Looped", &entry._looped))                 { _document._modified = true; }
         break;
      }
      case ActionType::SetSpriteVisible:
      {
         if (inputText("Sprite name", entry._name))                     { _document._modified = true; }
         if (ImGui::Checkbox("Visible", &entry._visible))               { _document._modified = true; }
         break;
      }
      case ActionType::MoveSprite:
      {
         if (inputText("Sprite name", entry._name))                     { _document._modified = true; }
         if (ImGui::InputFloat("Target X", &entry._x, 1.0f, 10.0f, "%.1f")) { _document._modified = true; }
         if (ImGui::InputFloat("Target Y", &entry._y, 1.0f, 10.0f, "%.1f")) { _document._modified = true; }
         if (ImGui::InputFloat("Speed (px/s)", &entry._speed, 1.0f, 10.0f, "%.1f")) { _document._modified = true; }
         if (inputText("Event (optional)", entry._event))               { _document._modified = true; }
         break;
      }
      case ActionType::FadeIn:
      case ActionType::FadeOut:
      {
         if (ImGui::InputFloat("Speed", &entry._speed, 0.1f, 1.0f, "%.2f")) { _document._modified = true; }
         break;
      }
      case ActionType::ShowDialogue:
      {
         if (inputText("Dialogue ID", entry._id))                       { _document._modified = true; }
         break;
      }
      case ActionType::SetPlayerVisible:
      case ActionType::SetInfoLayerVisible:
      {
         if (ImGui::Checkbox("Visible", &entry._visible))               { _document._modified = true; }
         break;
      }
      case ActionType::UnlockCamera:
      case ActionType::NextLevel:
      {
         ImGui::TextDisabled("No parameters.");
         break;
      }
   }
}
