#pragma once

#include "document.h"

#include <cstdint>
#include <string>

/// \brief top-level ImGui editor for deceptus cutscene JSON files.
///        renders a two-panel layout: an entry list on the left and a field
///        editor on the right. all file I/O goes through Document.
class Editor
{
public:
   /// \brief renders the full editor UI for one frame; call once per ImGui frame.
   void draw();

private:
   // -- panels --

   /// \brief renders the File menu bar including the current filepath display.
   void drawMenuBar();

   /// \brief renders the scrollable entry list and add/remove/reorder buttons.
   void drawEntryList();

   /// \brief renders the trigger, action-type, and field sections for the selected entry.
   void drawEntryEditor();

   /// \brief renders only the action-specific field widgets for the given entry.
   /// \param entry the entry whose fields are being edited; modified in place.
   void drawActionFields(CutsceneEntry& entry);

   // -- file I/O --

   /// \brief opens a Windows file dialog and loads the chosen JSON file.
   void openFile();

   /// \brief saves to _document._filepath, or delegates to saveFileAs() when no path is set.
   void saveFile();

   /// \brief opens a Windows save dialog and writes the document to the chosen path.
   void saveFileAs();

   // -- helpers --

   /// \brief wraps ImGui::InputText for std::string values.
   /// \param label widget label shown to the left of the input.
   /// \param value string to read from and write to.
   /// \param max_length maximum number of characters including the null terminator.
   /// \return true when the value was changed this frame.
   bool inputText(const char* label, std::string& value, size_t max_length = 256);

   // -- state --

   Document _document;                     //!< currently open cutscene document
   int32_t  _selected_entry_index = -1;    //!< index into _document._entries, -1 when nothing is selected
};
