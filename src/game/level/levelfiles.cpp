#include "levelfiles.h"

#include <filesystem>
#include <system_error>

void LevelFiles::clean(const LevelDescription& level_description)
{
   const auto path = std::filesystem::path(level_description._filename).parent_path();
   const auto crc_path = level_description._filename + ".crc";

   const auto filenames = {
      {"physics_grid_solid.png"},
      {"physics_grid_solid_onesided.png"},
      {"physics_path_solid.csv"},
      {"physics_path_solid.png"},
      {"physics_path_solid_onesided.csv"},
      {"physics_path_solid_onesided.png"},
      {"layer_level_solid.obj"},
      {"layer_level_solid_not_optimised.obj"},
      {"layer_level_solid_onesided_solid_onesided.obj"},
      {"layer_level_solid_onesided_solid_onesided_not_optimised.obj"},
      crc_path,
   };

   // the error_code overload rather than the throwing one: on the switch and in the browser the
   // level lives on a read-only filesystem, so every one of these removes fails with EROFS. That
   // came out as an uncaught filesystem_error which took the whole process down mid level load.
   // Dropping generated files is best effort anyway - if they cannot be removed there is nothing
   // stale to remove in the first place.
   for (const auto& filename : filenames)
   {
      std::error_code remove_error;
      std::filesystem::remove(path / filename, remove_error);
   }
}
