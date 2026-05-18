#pragma once

#include "databuffer.h"
#include "pathedge.h"
#include "pathsegments.h"
#include "pathvertex.h"

class PainterPath;

/// \brief Planar winged-edge graph built from one or two PainterPaths,
///        with all self-intersections resolved into graph vertices.
class WingedEdge
{
public:
   /// \brief Current position in a linked-list traversal around the graph.
   struct TraversalStatus
   {
      int edge = -1;                                                 //!< Current edge index.
      PathEdge::Traversal traversal = PathEdge::Traversal::Right;    //!< Which face side.
      PathEdge::Direction direction = PathEdge::Direction::Forward;  //!< Travel direction.

      void flipDirection();
      void flipTraversal();
      void flip();
   };

   WingedEdge();
   WingedEdge(const PainterPath& subject, const PainterPath& clip);

   void simplify();
   [[nodiscard]] PainterPath toPath() const;

   [[nodiscard]] int edgeCount() const;
   [[nodiscard]] PathEdge* edge(int edge_index);
   [[nodiscard]] const PathEdge* edge(int edge_index) const;

   [[nodiscard]] int vertexCount() const;
   [[nodiscard]] int addVertex(const PointF& point);
   [[nodiscard]] PathVertex* vertex(int vertex_index);
   [[nodiscard]] const PathVertex* vertex(int vertex_index) const;

   [[nodiscard]] TraversalStatus next(const TraversalStatus& status) const;

   [[nodiscard]] int addEdge(const PointF& point_a, const PointF& point_b);
   [[nodiscard]] int addEdge(int vertex_a, int vertex_b);

   [[nodiscard]] bool isInside(double x, double y) const;

   [[nodiscard]] static PathEdge::Traversal flip(PathEdge::Traversal traversal);
   [[nodiscard]] static PathEdge::Direction flip(PathEdge::Direction direction);

private:
   void intersectAndAdd();
   void removeEdge(int edge_index);
   int insert(const PathVertex& vertex);
   TraversalStatus findInsertStatus(int vertex_index, int edge_index) const;
   double delta(int vertex_index, int edge_a, int edge_b) const;

   DataBuffer<PathEdge> _edges;       //!< Flat edge storage.
   DataBuffer<PathVertex> _vertices;  //!< Flat vertex storage.
   PathSegments _segments;            //!< Segment list built from the input paths.
};

inline int WingedEdge::edgeCount() const
{
   return _edges.size();
}

inline PathEdge* WingedEdge::edge(int edge_index)
{
   return edge_index < 0 ? nullptr : &_edges.at(edge_index);
}

inline const PathEdge* WingedEdge::edge(int edge_index) const
{
   return edge_index < 0 ? nullptr : &_edges.at(edge_index);
}

inline int WingedEdge::vertexCount() const
{
   return _vertices.size();
}

inline int WingedEdge::addVertex(const PointF& point)
{
   _vertices << PathVertex(point);
   return _vertices.size() - 1;
}

inline PathVertex* WingedEdge::vertex(int vertex_index)
{
   return vertex_index < 0 ? nullptr : &_vertices.at(vertex_index);
}

inline const PathVertex* WingedEdge::vertex(int vertex_index) const
{
   return vertex_index < 0 ? nullptr : &_vertices.at(vertex_index);
}

inline PathEdge::Traversal WingedEdge::flip(PathEdge::Traversal traversal)
{
   return traversal == PathEdge::Traversal::Right ? PathEdge::Traversal::Left : PathEdge::Traversal::Right;
}

inline PathEdge::Direction WingedEdge::flip(PathEdge::Direction direction)
{
   return direction == PathEdge::Direction::Forward ? PathEdge::Direction::Backward : PathEdge::Direction::Forward;
}

inline void WingedEdge::TraversalStatus::flipTraversal()
{
   traversal = WingedEdge::flip(traversal);
}

inline void WingedEdge::TraversalStatus::flipDirection()
{
   direction = WingedEdge::flip(direction);
}

inline void WingedEdge::TraversalStatus::flip()
{
   flipDirection();
   flipTraversal();
}
