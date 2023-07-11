#include "levelfiles.h"

#include <filesystem>

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

   for (const auto& filename : filenames)
   {
      std::filesystem::remove(path / filename);
   }
}
