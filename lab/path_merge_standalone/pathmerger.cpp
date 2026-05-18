#include "pathmerger.h"

void PathMerger::addPolygon(const std::vector<PointF>& polygon)
{
   _path.addPolygon(polygon);
}

std::vector<std::vector<PointF>> PathMerger::simplified() const
{
   return _path.simplified().toSubpathPolygons();
}
