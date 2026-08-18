#pragma once

#include "cutsceneentry.h"

#include <string>
#include <vector>

/// \brief in-memory representation of a cutscene JSON file.
///        provides load and save operations that round-trip through the engine's
///        cutscene format (flat JSON array of at/on entries).
class Document
{
public:
   /// \brief loads a cutscene JSON file and populates _entries.
   /// \param path absolute or relative path to the JSON file.
   /// \return true on success; false if the file cannot be opened or parsed.
   bool load(const std::string& path);

   /// \brief serialises _entries to a JSON file compatible with cutscene.lua.
   /// \param path destination file path; created or overwritten.
   /// \return true on success; false if the file cannot be written.
   bool save(const std::string& path) const;

   /// \brief clears all entries and resets the filepath and modified flag.
   void reset();

   std::vector<CutsceneEntry> _entries;   //!< ordered list of cutscene entries
   std::string                _filepath;  //!< path of the currently open file, empty when unsaved
   bool                       _modified = false;  //!< true when there are unsaved changes
};
