#pragma once

#include <vector>

#include "pointf.h"

/// \brief A path composed of MoveTo and LineTo elements, mirroring the subset of
///        QPainterPath needed for polygon simplification.
class PainterPath
{
public:
   /// \brief Element type tag.
   enum class ElementType
   {
      MoveTo,
      LineTo
   };

   /// \brief A single path element: a typed 2D point.
   struct Element
   {
      ElementType type = ElementType::MoveTo;  //!< Whether this is a move or line.
      double x = 0.0;                          //!< X coordinate.
      double y = 0.0;                          //!< Y coordinate.
   };

   /// \brief Fill rule determining the interior of the path.
   enum class FillRule
   {
      OddEven,  //!< Standard even-odd rule.
      Winding   //!< Non-zero winding rule.
   };

   PainterPath() = default;

   void addPolygon(const std::vector<PointF>& polygon);

   void moveTo(double x, double y);
   void lineTo(double x, double y);

   [[nodiscard]] int elementCount() const;
   [[nodiscard]] const Element& elementAt(int index) const;

   [[nodiscard]] bool isEmpty() const;

   [[nodiscard]] FillRule fillRule() const;
   void setFillRule(FillRule rule);

   void setElementPositionAt(int index, double x, double y);

   [[nodiscard]] PainterPath simplified() const;
   [[nodiscard]] std::vector<std::vector<PointF>> toSubpathPolygons() const;

private:
   std::vector<Element> _elements;           //!< Ordered list of path elements.
   FillRule _fill_rule = FillRule::OddEven;  //!< Active fill rule.
};
