#pragma once

#include "databuffer.h"
#include "linef.h"
#include "painterpath.h"
#include "rectf.h"

#include <cstdint>

namespace PathMerge
{

/// \brief Builds a flat list of line segments from one or two PainterPaths
///        and computes pairwise segment intersections used by WingedEdge.
class PathSegments
{
public:
   /// \brief A segment-segment intersection event stored as a linked list per segment.
   struct Intersection
   {
      double t = 0.0;       //!< Parameter along the segment at the intersection (0..1).
      int32_t vertex = -1;  //!< Index of the intersection vertex in the point list.
      int32_t next = 0;     //!< Relative offset to the next Intersection node, or 0 if last.

      bool operator==(const Intersection& other) const
      {
         return t == other.t;
      }
      auto operator<=>(const Intersection& other) const
      {
         return t <=> other.t;
      }
   };

   /// \brief A directed line segment between two vertices, with its bounding box.
   struct Segment
   {
      Segment(int32_t path_id, int32_t vertex_a, int32_t vertex_b) : path(path_id), va(vertex_a), vb(vertex_b), intersection(-1)
      {
      }

      int32_t path = 0;           //!< Which input path this segment belongs to (0 or 1).
      int32_t va = -1;            //!< Index of the start vertex.
      int32_t vb = -1;            //!< Index of the end vertex.
      int32_t intersection = -1;  //!< Index into _intersections of the first event, or -1.
      RectF bounds;               //!< Axis-aligned bounding box of the segment.
   };

   explicit PathSegments(int32_t reserve_count);

   void setPath(const PainterPath& path);
   void addPath(const PainterPath& path);

   [[nodiscard]] int32_t intersections() const;
   [[nodiscard]] int32_t segments() const;
   [[nodiscard]] int32_t points() const;

   [[nodiscard]] const Segment& segmentAt(int32_t index) const;
   [[nodiscard]] LineF lineAt(int32_t index) const;
   [[nodiscard]] const RectF& elementBounds(int32_t index) const;
   [[nodiscard]] int32_t pathId(int32_t index) const;

   [[nodiscard]] const PointF& pointAt(int32_t vertex) const;
   [[nodiscard]] int32_t addPoint(const PointF& point);

   [[nodiscard]] const Intersection* intersectionAt(int32_t index) const;
   void addIntersection(int32_t index, const Intersection& intersection);

   void mergePoints();

private:
   DataBuffer<PointF> _points;               //!< Deduplicated vertex list.
   DataBuffer<Segment> _segments;            //!< Flat segment list.
   DataBuffer<Intersection> _intersections;  //!< Packed intersection events.
   int32_t _path_id = 0;                     //!< Counter incremented per addPath call.
};

inline int32_t PathSegments::segments() const
{
   return _segments.size();
}

inline int32_t PathSegments::points() const
{
   return _points.size();
}

inline int32_t PathSegments::intersections() const
{
   return _intersections.size();
}

inline const PointF& PathSegments::pointAt(int32_t index) const
{
   return _points.at(index);
}

inline int32_t PathSegments::addPoint(const PointF& point)
{
   _points << point;
   return _points.size() - 1;
}

inline const PathSegments::Segment& PathSegments::segmentAt(int32_t index) const
{
   return _segments.at(index);
}

inline LineF PathSegments::lineAt(int32_t index) const
{
   const Segment& seg = _segments.at(index);
   return {_points.at(seg.va), _points.at(seg.vb)};
}

inline const RectF& PathSegments::elementBounds(int32_t index) const
{
   return _segments.at(index).bounds;
}

inline int32_t PathSegments::pathId(int32_t index) const
{
   return _segments.at(index).path;
}

inline const PathSegments::Intersection* PathSegments::intersectionAt(int32_t index) const
{
   const auto isect_index = _segments.at(index).intersection;
   if (isect_index < 0)
   {
      return nullptr;
   }
   return &_intersections.at(isect_index);
}

inline void PathSegments::addIntersection(int32_t index, const Intersection& intersection)
{
   _intersections << intersection;
   Segment& seg = _segments.at(index);
   if (seg.intersection < 0)
   {
      seg.intersection = _intersections.size() - 1;
   }
   else
   {
      Intersection* existing = &_intersections.at(seg.intersection);
      while (existing->next != 0)
      {
         existing += existing->next;
      }
      existing->next = (_intersections.size() - 1) - (existing - _intersections.data());
   }
}

}  // namespace PathMerge
