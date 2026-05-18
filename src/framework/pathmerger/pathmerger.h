#pragma once

#include "painterpath.h"
#include "pointf.h"

#include <string>
#include <vector>

namespace PathMerge
{

/// \brief Merges and simplifies a collection of polygons using the winged-edge boolean union.
class PathMerger
{
public:
   /// \brief Counts collected across a load/save round-trip.
   struct Stats
   {
      size_t points_in = 0;   //!< Unique vertices in the input OBJ.
      size_t points_out = 0;  //!< Unique vertices written to the output OBJ.
      size_t faces_in = 0;    //!< Faces in the input OBJ.
      size_t faces_out = 0;   //!< Faces written to the output OBJ.
   };

   void loadObj(const std::string& filename);

   void addPolygon(const std::vector<PointF>& polygon);

   [[nodiscard]] std::vector<std::vector<PointF>> simplified() const;

   Stats saveObj(const std::string& filename) const;

private:
   PainterPath _path;        //!< Accumulated input path.
   size_t _points_in = 0;   //!< Vertex count from the last loadObj call.
   size_t _faces_in = 0;    //!< Face count from the last loadObj call.
};

}  // namespace PathMerge
