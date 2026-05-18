#pragma once

#include "databuffer.h"
#include "linef.h"
#include "painterpath.h"
#include "rectf.h"

/// \brief Builds a flat list of line segments from one or two PainterPaths
///        and computes pairwise segment intersections used by WingedEdge.
class PathSegments
{
public:
   /// \brief A segment-segment intersection event stored as a linked list per segment.
   struct Intersection
   {
      double t = 0.0;   //!< Parameter along the segment at the intersection (0..1).
      int vertex = -1;  //!< Index of the intersection vertex in the point list.
      int next = 0;     //!< Relative offset to the next Intersection node, or 0 if last.

      bool operator<(const Intersection& other) const { return t < other.t; }
   };

   /// \brief A directed line segment between two vertices, with its bounding box.
   struct Segment
   {
      Segment(int path_id, int vertex_a, int vertex_b)
         : path(path_id)
         , va(vertex_a)
         , vb(vertex_b)
         , intersection(-1)
      {
      }

      int path = 0;          //!< Which input path this segment belongs to (0 or 1).
      int va = -1;           //!< Index of the start vertex.
      int vb = -1;           //!< Index of the end vertex.
      int intersection = -1; //!< Index into _intersections of the first event, or -1.
      RectF bounds;          //!< Axis-aligned bounding box of the segment.
   };

   explicit PathSegments(int reserve_count);

   void setPath(const PainterPath& path);
   void addPath(const PainterPath& path);

   int intersections() const;
   int segments() const;
   int points() const;

   const Segment& segmentAt(int index) const;
   LineF lineAt(int index) const;
   const RectF& elementBounds(int index) const;
   int pathId(int index) const;

   const PointF& pointAt(int vertex) const;
   int addPoint(const PointF& point);

   const Intersection* intersectionAt(int index) const;
   void addIntersection(int index, const Intersection& intersection);

   void mergePoints();

private:
   DataBuffer<PointF> _points;                //!< Deduplicated vertex list.
   DataBuffer<Segment> _segments;             //!< Flat segment list.
   DataBuffer<Intersection> _intersections;   //!< Packed intersection events.
   int _path_id = 0;                          //!< Counter incremented per addPath call.
};

inline int PathSegments::segments() const
{
   return _segments.size();
}

inline int PathSegments::points() const
{
   return _points.size();
}

inline int PathSegments::intersections() const
{
   return _intersections.size();
}

inline const PointF& PathSegments::pointAt(int index) const
{
   return _points.at(index);
}

inline int PathSegments::addPoint(const PointF& point)
{
   _points << point;
   return _points.size() - 1;
}

inline const PathSegments::Segment& PathSegments::segmentAt(int index) const
{
   return _segments.at(index);
}

inline LineF PathSegments::lineAt(int index) const
{
   const Segment& seg = _segments.at(index);
   return {_points.at(seg.va), _points.at(seg.vb)};
}

inline const RectF& PathSegments::elementBounds(int index) const
{
   return _segments.at(index).bounds;
}

inline int PathSegments::pathId(int index) const
{
   return _segments.at(index).path;
}

inline const PathSegments::Intersection* PathSegments::intersectionAt(int index) const
{
   const int isect_index = _segments.at(index).intersection;
   if (isect_index < 0)
   {
      return nullptr;
   }
   return &_intersections.at(isect_index);
}

inline void PathSegments::addIntersection(int index, const Intersection& intersection)
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
