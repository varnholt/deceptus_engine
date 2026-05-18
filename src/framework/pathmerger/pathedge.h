#pragma once

#include <utility>

namespace PathMerge
{

/// \brief A directed edge in the winged-edge planar graph.
class PathEdge
{
public:
   enum class Traversal
   {
      Right,
      Left
   };

   enum class Direction
   {
      Forward,
      Backward
   };

   enum class Type
   {
      Line,
      Curve
   };

   explicit PathEdge(int vertex_a = -1, int vertex_b = -1);

   [[nodiscard]] int next(Traversal traversal, Direction direction) const;
   void setNext(Traversal traversal, Direction direction, int next_edge);
   void setNext(Direction direction, int next_edge);

   [[nodiscard]] Direction directionTo(int vertex) const;
   [[nodiscard]] int vertex(Direction direction) const;

   mutable int flag = 0;    //!< Traversal flag used during path extraction.
   int winding_a = 0;       //!< Winding contribution from path A.
   int winding_b = 0;       //!< Winding contribution from path B.
   int first = -1;          //!< Index of the tail vertex.
   int second = -1;         //!< Index of the head vertex.
   double angle = 0.0;      //!< Angle of the edge from its tail vertex.
   double inv_angle = 0.0;  //!< Angle in the reverse (head-to-tail) direction.

private:
   int _next[2][2] = {{-1, -1}, {-1, -1}};  //!< Linked-list next edges: [traversal][direction].
};

inline PathEdge::PathEdge(int vertex_a, int vertex_b) : first(vertex_a), second(vertex_b)
{
}

inline int PathEdge::next(Traversal traversal, Direction direction) const
{
   return _next[std::to_underlying(traversal)][std::to_underlying(direction)];
}

inline void PathEdge::setNext(Traversal traversal, Direction direction, int next_edge)
{
   _next[std::to_underlying(traversal)][std::to_underlying(direction)] = next_edge;
}

inline void PathEdge::setNext(Direction direction, int next_edge)
{
   _next[0][std::to_underlying(direction)] = next_edge;
   _next[1][std::to_underlying(direction)] = next_edge;
}

inline PathEdge::Direction PathEdge::directionTo(int vertex) const
{
   return first == vertex ? Direction::Backward : Direction::Forward;
}

inline int PathEdge::vertex(Direction direction) const
{
   return direction == Direction::Backward ? first : second;
}

}  // namespace PathMerge
