#include "settingsui.h"

#include "imgui/imgui.h"

#include <algorithm>

namespace
{

//!< the help belonging to the row the pointer is over, or to the widget being dragged. Cleared at the
//!< start of every frame and filled in by whichever row reports itself hovered or active
struct HoveredRow
{
   bool _present{false};
   std::string _name;
   std::string _range;
   const char* _what{nullptr};
   const char* _note{nullptr};
};

//!< being filled in during the frame currently being drawn
HoveredRow __hovered;

//!< what the pane showed on the previous frame. The pane's height has to be reserved before the rows
//!< are drawn, and nothing is known about what will be pointed at until they have been - so the height
//!< is measured from the previous frame's text. One frame of lag on it is not visible, and it beats a
//!< fixed height, which either clips the longer notes or reserves empty space for the shorter ones
HoveredRow __shown;

/// \brief how much room the pane needs for the text it is about to show, wrapping included.
float helpHeight(float wrap_width_px)
{
   const auto line_height = ImGui::GetTextLineHeightWithSpacing();
   if (!__shown._present)
   {
      return line_height + ImGui::GetStyle().ItemSpacing.y * 2.0f;
   }

   auto height = line_height;  // the name and its range
   for (const auto* text : {__shown._what, __shown._note})
   {
      if (text != nullptr)
      {
         height += ImGui::CalcTextSize(text, nullptr, false, wrap_width_px).y + ImGui::GetStyle().ItemSpacing.y;
      }
   }

   return height + ImGui::GetStyle().ItemSpacing.y * 2.0f;
}

/// \brief records the row's help if any of its widgets is hovered or being dragged.
/// \note called after each of the row's widgets, because ImGui reports hover for the item just drawn.
///       Active as well as hovered: a slider being dragged stops being hovered as soon as the pointer
///       leaves it, and the value is still the one being changed.
void captureIfPointedAt(const std::string& name, const std::string& range, const char* what, const char* note)
{
   if (!ImGui::IsItemHovered() && !ImGui::IsItemActive())
   {
      return;
   }

   __hovered = HoveredRow{._present = true, ._name = name, ._range = range, ._what = what, ._note = note};
}

std::string formatRange(float min, float max)
{
   char buffer[64];
   std::snprintf(buffer, sizeof(buffer), "%g to %g", min, max);
   return buffer;
}

std::string formatRange(int32_t min, int32_t max)
{
   char buffer[64];
   std::snprintf(buffer, sizeof(buffer), "%d to %d", min, max);
   return buffer;
}

}  // namespace

void SettingsUi::beginSettings()
{
   __hovered = HoveredRow{};

   // the rows scroll, the help pane does not. A negative height leaves the child that much short of
   // the bottom of the window, which is what keeps the pane in place
   ImGui::BeginChild("settings", ImVec2{0.0f, -helpHeight(ImGui::GetContentRegionAvail().x)}, false);
}

void SettingsUi::endSettings()
{
   ImGui::EndChild();
   ImGui::Separator();

   __shown = __hovered;

   if (!__shown._present)
   {
      ImGui::TextDisabled("point at a setting to read what it does");
      return;
   }

   ImGui::TextUnformatted(__shown._name.c_str());
   ImGui::SameLine();
   ImGui::TextDisabled("(%s)", __shown._range.c_str());
   ImGui::TextWrapped("%s", __shown._what != nullptr ? __shown._what : "");

   if (__shown._note != nullptr)
   {
      ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
      ImGui::TextWrapped("%s", __shown._note);
      ImGui::PopStyleColor();
   }
}

void SettingsUi::drawFloat(const std::string& name, float* value, float min, float max, const char* what, const char* note)
{
   const auto range = formatRange(min, max);
   const auto input_id = "##" + name + "_input";
   const auto slider_id = "##" + name + "_slider";

   ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.2f);

   if (ImGui::InputFloat(input_id.c_str(), value, 0.1f, 1.0f, "%.3f"))
   {
      *value = std::clamp(*value, min, max);
   }
   captureIfPointedAt(name, range, what, note);

   ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.35f);
   ImGui::SameLine();
   ImGui::SliderFloat(slider_id.c_str(), value, min, max);
   captureIfPointedAt(name, range, what, note);

   ImGui::SameLine();
   ImGui::TextUnformatted(name.c_str());
   captureIfPointedAt(name, range, what, note);
}

void SettingsUi::drawInt(const std::string& name, int32_t* value, int32_t min, int32_t max, const char* what, const char* note)
{
   const auto range = formatRange(min, max);
   const auto input_id = "##" + name + "_input";
   const auto slider_id = "##" + name + "_slider";

   ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.2f);

   if (ImGui::InputInt(input_id.c_str(), value))
   {
      *value = std::clamp(*value, min, max);
   }
   captureIfPointedAt(name, range, what, note);

   ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.35f);
   ImGui::SameLine();
   ImGui::SliderInt(slider_id.c_str(), value, min, max);
   captureIfPointedAt(name, range, what, note);

   ImGui::SameLine();
   ImGui::TextUnformatted(name.c_str());
   captureIfPointedAt(name, range, what, note);
}

void SettingsUi::drawBool(const std::string& name, bool* value, const char* what, const char* note)
{
   ImGui::Checkbox(name.c_str(), value);
   captureIfPointedAt(name, "on or off", what, note);
}
