#ifndef PARSEDATA_H
#define PARSEDATA_H

#include <cstdint>
#include <string>
#include <vector>

#include "constants.h"

struct ParseData
{
   std::string filename_obj_optimized;
   std::string filename_obj_not_optimized;
   std::string filename_physics_path_csv;
   std::string filename_grid_image;
   std::string filename_path_image;
   ObjectType object_type = ObjectTypeSolid;
   std::vector<int32_t> colliding_tiles;
};

#endif  // PARSEDATA_H
