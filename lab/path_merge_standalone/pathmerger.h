#pragma once

#include "painterpath.h"
#include "pointf.h"

#include <vector>

/// \brief Merges and simplifies a collection of polygons using the winged-edge boolean union.
class PathMerger
{
public:
   void addPolygon(const std::vector<PointF>& polygon);

   [[nodiscard]] std::vector<std::vector<PointF>> simplified() const;

private:
   PainterPath _path;  //!< Accumulated input path.
};
