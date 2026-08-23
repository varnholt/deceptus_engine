#ifndef SETTINGSUI_H
#define SETTINGSUI_H

#include <cstdint>
#include <string>

/// \brief the settings rows the configuration editors are built from, and the help pane under them.
///
/// Every row carries its own explanation, and the pane at the bottom of the window shows the one
/// belonging to whatever the pointer is over - or to whatever slider is being dragged, which is the
/// reason this is a pane and not a tooltip: a tooltip disappears the moment a drag starts, which is
/// exactly when the reader wants to know what the value does, and it covers the neighbouring rows
/// while it is up.
///
/// The rows were previously a pair of lambdas copied into each editor verbatim. They live here so
/// that a row only has to learn about its explanation once.
namespace SettingsUi
{

/// \brief opens the scrolling region the rows are drawn into and forgets the previous frame's help.
/// \note the region stops short of the bottom of the window by exactly the height of the help pane,
///       so the pane stays put instead of scrolling away with the rows.
void beginSettings();

/// \brief closes the scrolling region and draws the help pane beneath it.
void endSettings();

/// \brief draws a float row: a number field, a slider, and the name.
/// \param name row label, also the id the two widgets are keyed on.
/// \param value edited in place; typing into the number field is clamped to the range.
/// \param min lowest value the slider offers.
/// \param max highest value the slider offers.
/// \param what one sentence on what the value does, shown in the help pane.
/// \param note what to watch out for: units, what it interacts with, when it takes effect. Optional.
void drawFloat(const std::string& name, float* value, float min, float max, const char* what, const char* note = nullptr);

/// \brief draws an integer row: a number field, a slider, and the name.
/// \param name row label, also the id the two widgets are keyed on.
/// \param value edited in place; typing into the number field is clamped to the range.
/// \param min lowest value the slider offers.
/// \param max highest value the slider offers.
/// \param what one sentence on what the value does, shown in the help pane.
/// \param note what to watch out for: units, what it interacts with, when it takes effect. Optional.
void drawInt(const std::string& name, int32_t* value, int32_t min, int32_t max, const char* what, const char* note = nullptr);

/// \brief draws a checkbox row.
/// \param name checkbox label.
/// \param value edited in place.
/// \param what one sentence on what the value does, shown in the help pane.
/// \param note what to watch out for: units, what it interacts with, when it takes effect. Optional.
void drawBool(const std::string& name, bool* value, const char* what, const char* note = nullptr);

}  // namespace SettingsUi

#endif  // SETTINGSUI_H
