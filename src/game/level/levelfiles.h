#ifndef LEVELFILES_H
#define LEVELFILES_H

#include "leveldescription.h"

namespace LevelFiles
{
/// \brief removes generated physics and checksum files for a level directory.
/// \param level_description level metadata containing the source filename used to resolve the folder.
void clean(const LevelDescription& level_description);

};

#endif  // LEVELFILES_H
