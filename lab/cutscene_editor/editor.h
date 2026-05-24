#pragma once

#include "document.h"

#include <cstdint>
#include <string>

class Editor
{
public:
   void draw();

private:
   void drawMenuBar();
   void drawEntryList();
   void drawEntryEditor();
   void drawActionFields(CutsceneEntry& entry);

   bool inputText(const char* label, std::string& value, size_t max_length = 256);

   void openFile();
   void saveFile();
   void saveFileAs();

   Document _document;
   int32_t  _selected_entry_index = -1;
};
