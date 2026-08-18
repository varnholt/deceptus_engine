#include <print>

#include "pathmerger.h"

int main(int32_t argc, char** argv)
{
   if (argc != 3)
   {
      std::println("usage: {} input_file.obj output_file.obj", argv[0]);
      exit(0);
   }

   // auto in = "C:\\git\\build\\layer_level.obj";
   // auto out = "C:\\git\\build\\layer_level_out.obj";

   PathMerger merger;
   merger.loadObj(argv[1]);

   const auto stats = merger.saveObj(argv[2]);

   std::println(
      "optimised mesh written to '{}', points: {} -> {}, faces: {} -> {}, factor: {:g}",
      argv[2],
      stats.points_in,
      stats.points_out,
      stats.faces_in,
      stats.faces_out,
      stats.points_out / static_cast<float>(stats.points_in)
   );

   return 0;
}
