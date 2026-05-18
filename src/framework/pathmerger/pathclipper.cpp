#include "pathclipper.h"

#include "painterpath.h"
#include "wingededge.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <ranges>
#include <vector>

namespace PathMerge
{

namespace
{

inline bool fuzzyCompare(double first, double second)
{
   return std::abs(first - second) * 1000000000000.0 <= std::min(std::abs(first), std::abs(second));
}

// Marks a connected face as inside (to be included in the result).
void traverseFace(WingedEdge& graph, int edge_index, PathEdge::Traversal traversal)
{
   WingedEdge::TraversalStatus status;
   status.edge = edge_index;
   status.traversal = traversal;
   status.direction = PathEdge::Direction::Forward;

   do
   {
      const int face_flag = status.traversal == PathEdge::Traversal::Left ? 1 : 2;
      PathEdge* edge_ptr = graph.edge(status.edge);
      edge_ptr->flag |= (face_flag | (face_flag << 4));
      status = graph.next(status);
   } while (status.edge != edge_index);
}

// Marks a connected face as outside (not included in the result).
void clearFace(WingedEdge& graph, int edge_index, PathEdge::Traversal traversal)
{
   WingedEdge::TraversalStatus status;
   status.edge = edge_index;
   status.traversal = traversal;
   status.direction = PathEdge::Direction::Forward;

   do
   {
      const int face_flag = status.traversal == PathEdge::Traversal::Left ? 1 : 2;
      graph.edge(status.edge)->flag |= face_flag;
      status = graph.next(status);
   } while (status.edge != edge_index);
}

// An edge that crosses the sweep line at a given x coordinate.
struct CrossingEdge
{
   int edge = -1;   //!< Index of the crossing edge.
   double x = 0.0;  //!< X coordinate of the crossing.

   bool operator==(const CrossingEdge& other) const
   {
      return x == other.x;
   }
   auto operator<=>(const CrossingEdge& other) const
   {
      return x <=> other.x;
   }
};

// Gathers all edges that straddle the given y coordinate.
std::vector<CrossingEdge> findCrossings(const WingedEdge& graph, double y)
{
   std::vector<CrossingEdge> crossings;
   for (int edge_index = 0; edge_index < graph.edgeCount(); ++edge_index)
   {
      const PathEdge* edge_ptr = graph.edge(edge_index);
      const PathVertex* vertex_a = graph.vertex(edge_ptr->first);
      const PathVertex* vertex_b = graph.vertex(edge_ptr->second);

      if ((vertex_a->y < y && vertex_b->y > y) || (vertex_a->y > y && vertex_b->y < y))
      {
         const double intersection_x = vertex_a->x + (vertex_b->x - vertex_a->x) * (y - vertex_a->y) / (vertex_b->y - vertex_a->y);
         crossings.push_back({edge_index, intersection_x});
      }
   }
   return crossings;
}

}  // namespace

PathClipper::PathClipper(const PainterPath& subject, const PainterPath& clip) : _subject_path(subject), _clip_path(clip)
{
   _a_mask = subject.fillRule() == PainterPath::FillRule::Winding ? ~0 : 0x1;
   _b_mask = clip.fillRule() == PainterPath::FillRule::Winding ? ~0 : 0x1;
}

PainterPath PathClipper::clip(Operation operation)
{
   _operation = operation;

   WingedEdge graph(_subject_path, _clip_path);

   doClip(graph, ClipperMode::Clip);

   return graph.toPath();
}

bool PathClipper::doClip(WingedEdge& graph, ClipperMode mode)
{
   std::vector<double> y_coordinates;
   y_coordinates.reserve(graph.vertexCount());
   for (int vertex_index = 0; vertex_index < graph.vertexCount(); ++vertex_index)
   {
      y_coordinates.push_back(graph.vertex(vertex_index)->y);
   }

   std::ranges::sort(y_coordinates);
   y_coordinates.erase(std::ranges::unique(y_coordinates, fuzzyCompare).begin(), y_coordinates.end());

   bool found = false;
   do
   {
      found = false;
      int best_edge_index = 0;
      double max_height = 0.0;

      for (int edge_index = 0; edge_index < graph.edgeCount(); ++edge_index)
      {
         PathEdge* edge_ptr = graph.edge(edge_index);

         if ((edge_ptr->flag & 0x3) == 0x3)
         {
            continue;
         }

         const PathVertex* vertex_a = graph.vertex(edge_ptr->first);
         const PathVertex* vertex_b = graph.vertex(edge_ptr->second);

         if (fuzzyCompare(vertex_a->y, vertex_b->y))
         {
            continue;
         }

         found = true;

         const double edge_height = std::abs(vertex_a->y - vertex_b->y);
         if (edge_height > max_height)
         {
            best_edge_index = edge_index;
            max_height = edge_height;
         }
      }

      if (found)
      {
         PathEdge* edge_ptr = graph.edge(best_edge_index);
         const PathVertex* vertex_a = graph.vertex(edge_ptr->first);
         const PathVertex* vertex_b = graph.vertex(edge_ptr->second);

         const double min_y = std::min(vertex_a->y, vertex_b->y);
         const double max_y = std::max(vertex_a->y, vertex_b->y);

         auto fuzzy_find = [&](double target) -> std::vector<double>::iterator {
            return std::find_if(
               y_coordinates.begin(), y_coordinates.end(), [target](double value) { return fuzzyCompare(value, target); }
            );
         };

         const auto first_iter = fuzzy_find(min_y);
         const auto last_iter = fuzzy_find(max_y);

         assert(first_iter != y_coordinates.end());
         assert(first_iter != y_coordinates.end() - 1);

         const int first_idx = static_cast<int>(first_iter - y_coordinates.begin());
         const int last_idx = static_cast<int>(last_iter - y_coordinates.begin());

         double biggest_gap = y_coordinates[first_idx + 1] - y_coordinates[first_idx];
         int best_gap_idx = first_idx;

         for (int gap_idx = first_idx + 1; gap_idx < last_idx; ++gap_idx)
         {
            const double gap = y_coordinates[gap_idx + 1] - y_coordinates[gap_idx];
            if (gap > biggest_gap)
            {
               best_gap_idx = gap_idx;
               biggest_gap = gap;
            }
         }

         const double sweep_y = 0.5 * (y_coordinates[best_gap_idx] + y_coordinates[best_gap_idx + 1]);

         if (handleCrossingEdges(graph, sweep_y, mode) && mode == ClipperMode::Check)
         {
            return true;
         }

         edge_ptr->flag |= 0x3;
      }
   } while (found);

   if (mode == ClipperMode::Clip)
   {
      graph.simplify();
   }

   return false;
}

bool PathClipper::handleCrossingEdges(WingedEdge& graph, double y, ClipperMode mode)
{
   std::vector<CrossingEdge> crossings = findCrossings(graph, y);

   assert(!crossings.empty());
   std::ranges::sort(crossings);

   int winding_a = 0;
   int winding_b = 0;
   int winding_d = 0;

   for (int crossing_index = 0; crossing_index < static_cast<int>(crossings.size()) - 1; ++crossing_index)
   {
      const int edge_index = crossings[crossing_index].edge;
      const PathEdge* edge_ptr = graph.edge(edge_index);

      winding_a += edge_ptr->winding_a;
      winding_b += edge_ptr->winding_b;

      const bool has_left = (edge_ptr->flag >> 4) & 1;
      const bool has_right = (edge_ptr->flag >> 4) & 2;

      winding_d += static_cast<int>(has_left) ^ static_cast<int>(has_right);

      const bool in_a = (winding_a & _a_mask) != 0;
      const bool in_b = (winding_b & _b_mask) != 0;
      const bool in_d = (winding_d & 0x1) != 0;

      bool inside_result = false;
      switch (_operation)
      {
         case Operation::BoolAnd:
            inside_result = in_a && in_b;
            break;
         case Operation::BoolOr:
         case Operation::Simplify:
            inside_result = in_a || in_b;
            break;
         case Operation::BoolSub:
            inside_result = in_a && !in_b;
            break;
      }

      const bool should_add = in_d ^ inside_result;

      if (should_add)
      {
         if (mode == ClipperMode::Check)
         {
            return true;
         }

         const double y0 = graph.vertex(edge_ptr->first)->y;
         const double y1 = graph.vertex(edge_ptr->second)->y;

         if (y0 < y1)
         {
            if (!(edge_ptr->flag & 1))
            {
               traverseFace(graph, edge_index, PathEdge::Traversal::Left);
            }
            if (!(edge_ptr->flag & 2))
            {
               clearFace(graph, edge_index, PathEdge::Traversal::Right);
            }
         }
         else
         {
            if (!(edge_ptr->flag & 1))
            {
               clearFace(graph, edge_index, PathEdge::Traversal::Left);
            }
            if (!(edge_ptr->flag & 2))
            {
               traverseFace(graph, edge_index, PathEdge::Traversal::Right);
            }
         }

         ++winding_d;
      }
      else
      {
         if (!(edge_ptr->flag & 1))
         {
            clearFace(graph, edge_index, PathEdge::Traversal::Left);
         }
         if (!(edge_ptr->flag & 2))
         {
            clearFace(graph, edge_index, PathEdge::Traversal::Right);
         }
      }
   }

   return false;
}

}  // namespace PathMerge
