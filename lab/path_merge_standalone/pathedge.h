#pragma once

#include <cstdint>
#include <utility>

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

   explicit PathEdge(int32_t vertex_a = -1, int32_t vertex_b = -1);

   [[nodiscard]] int32_t next(Traversal traversal, Direction direction) const;
   void setNext(Traversal traversal, Direction direction, int32_t next_edge);
   void setNext(Direction direction, int32_t next_edge);

   [[nodiscard]] Direction directionTo(int32_t vertex) const;
   [[nodiscard]] int32_t vertex(Direction direction) const;

   mutable int32_t flag = 0;  //!< Traversal flag used during path extraction.
   int32_t winding_a = 0;     //!< Winding contribution from path A.
   int32_t winding_b = 0;     //!< Winding contribution from path B.
   int32_t first = -1;        //!< Index of the tail vertex.
   int32_t second = -1;       //!< Index of the head vertex.
   double angle = 0.0;        //!< Angle of the edge from its tail vertex.
   double inv_angle = 0.0;    //!< Angle in the reverse (head-to-tail) direction.

private:
   int32_t _next[2][2] = {{-1, -1}, {-1, -1}};  //!< Linked-list next edges: [traversal][direction].
};

inline PathEdge::PathEdge(int32_t vertex_a, int32_t vertex_b) : first(vertex_a), second(vertex_b)
{
}

inline int32_t PathEdge::next(Traversal traversal, Direction direction) const
{
   return _next[std::to_underlying(traversal)][std::to_underlying(direction)];
}

inline void PathEdge::setNext(Traversal traversal, Direction direction, int32_t next_edge)
{
   _next[std::to_underlying(traversal)][std::to_underlying(direction)] = next_edge;
}

inline void PathEdge::setNext(Direction direction, int32_t next_edge)
{
   _next[0][std::to_underlying(direction)] = next_edge;
   _next[1][std::to_underlying(direction)] = next_edge;
}

inline PathEdge::Direction PathEdge::directionTo(int32_t vertex) const
{
   return first == vertex ? Direction::Backward : Direction::Forward;
}

inline int32_t PathEdge::vertex(Direction direction) const
{
   return direction == Direction::Backward ? first : second;
}
