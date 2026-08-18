#include "painterpath.h"

#include "pathclipper.h"

#include <cstdint>

namespace PathMerge
{

void PainterPath::addPolygon(const std::vector<PointF>& polygon)
{
   if (polygon.empty())
   {
      return;
   }
   moveTo(polygon[0].x, polygon[0].y);
   for (auto point_index = size_t{1}; point_index < polygon.size(); ++point_index)
   {
      lineTo(polygon[point_index].x, polygon[point_index].y);
   }
}

void PainterPath::moveTo(double x, double y)
{
   _elements.push_back({ElementType::MoveTo, x, y});
}

void PainterPath::lineTo(double x, double y)
{
   _elements.push_back({ElementType::LineTo, x, y});
}

int32_t PainterPath::elementCount() const
{
   return static_cast<int32_t>(_elements.size());
}

const PainterPath::Element& PainterPath::elementAt(int32_t index) const
{
   return _elements[index];
}

bool PainterPath::isEmpty() const
{
   return _elements.empty();
}

PainterPath::FillRule PainterPath::fillRule() const
{
   return _fill_rule;
}

void PainterPath::setFillRule(FillRule rule)
{
   _fill_rule = rule;
}

void PainterPath::setElementPositionAt(int32_t index, double x, double y)
{
   _elements[index].x = x;
   _elements[index].y = y;
}

PainterPath PainterPath::simplified() const
{
   if (isEmpty())
   {
      return *this;
   }
   PathClipper clipper(*this, PainterPath{});
   return clipper.clip(PathClipper::Operation::Simplify);
}

std::vector<std::vector<PointF>> PainterPath::toSubpathPolygons() const
{
   std::vector<std::vector<PointF>> result;
   std::vector<PointF> current;

   for (const auto& element : _elements)
   {
      if (element.type == ElementType::MoveTo)
      {
         if (current.size() > 1)
         {
            result.push_back(current);
         }
         current.clear();
         current.push_back({element.x, element.y});
      }
      else
      {
         current.push_back({element.x, element.y});
      }
   }

   if (current.size() > 1)
   {
      result.push_back(current);
   }

   return result;
}

}  // namespace PathMerge
