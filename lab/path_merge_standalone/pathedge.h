#pragma once

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

   int next(Traversal traversal, Direction direction) const;
   void setNext(Traversal traversal, Direction direction, int next_edge);
   void setNext(Direction direction, int next_edge);

   Direction directionTo(int vertex) const;
   int vertex(Direction direction) const;

   mutable int flag = 0;  //!< Traversal flag used during path extraction.
   int winding_a = 0;     //!< Winding contribution from path A.
   int winding_b = 0;     //!< Winding contribution from path B.
   int first = -1;        //!< Index of the tail vertex.
   int second = -1;       //!< Index of the head vertex.
   double angle = 0.0;    //!< Angle of the edge from its tail vertex.
   double inv_angle = 0.0; //!< Angle in the reverse (head-to-tail) direction.

private:
   int _next[2][2] = {{-1, -1}, {-1, -1}}; //!< Linked-list next edges: [traversal][direction].
};

inline PathEdge::PathEdge(int vertex_a, int vertex_b)
   : first(vertex_a)
   , second(vertex_b)
{
}

inline int PathEdge::next(Traversal traversal, Direction direction) const
{
   return _next[static_cast<int>(traversal)][static_cast<int>(direction)];
}

inline void PathEdge::setNext(Traversal traversal, Direction direction, int next_edge)
{
   _next[static_cast<int>(traversal)][static_cast<int>(direction)] = next_edge;
}

inline void PathEdge::setNext(Direction direction, int next_edge)
{
   _next[0][static_cast<int>(direction)] = next_edge;
   _next[1][static_cast<int>(direction)] = next_edge;
}

inline PathEdge::Direction PathEdge::directionTo(int vertex) const
{
   return first == vertex ? Direction::Backward : Direction::Forward;
}

inline int PathEdge::vertex(Direction direction) const
{
   return direction == Direction::Backward ? first : second;
}
