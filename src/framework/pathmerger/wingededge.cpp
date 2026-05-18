#include "wingededge.h"

#include "mathhelpers.h"
#include "painterpath.h"

#include <algorithm>
#include <cassert>
#include <climits>
#include <cmath>
#include <cstdint>
#include <limits>
#include <ranges>
#include <vector>

namespace PathMerge
{

namespace
{

inline bool points_are_close(const PointF& point_a, const PointF& point_b)
{
   return MathHelpers::fuzzyIsNull(point_a.x - point_b.x) && MathHelpers::fuzzyIsNull(point_a.y - point_b.y);
}

inline bool fuzzy_is_null(double value)
{
   return std::abs(value) <= 1e-12;
}

inline bool fuzzy_compare(double first, double second)
{
   return std::abs(first - second) * 1000000000000.0 <= std::min(std::abs(first), std::abs(second));
}

inline double dot(const PointF& vector_a, const PointF& vector_b)
{
   return vector_a.x * vector_b.x + vector_a.y * vector_b.y;
}

inline void normalize_in_place(double& x, double& y)
{
   const double reciprocal = 1.0 / std::sqrt(x * x + y * y);
   x *= reciprocal;
   y *= reciprocal;
}

// ---------------------------------------------------------------------------
// Segment-segment intersection value  (local to this translation unit)
// ---------------------------------------------------------------------------

struct SegmentIntersection
{
   double alpha_a = 0.0;  //!< Parameter along segment A (0..1).
   double alpha_b = 0.0;  //!< Parameter along segment B (0..1).
   PointF pos;            //!< Position of the intersection.
};

// ---------------------------------------------------------------------------
// SegmentTree  — interval tree for O(n log n) pairwise intersection finding
// ---------------------------------------------------------------------------

struct TreeNode
{
   double split_left = 0.0;
   double split_right = 0.0;
   bool leaf = false;

   int32_t lowest_left_index = 0;
   int32_t lowest_right_index = 0;

   union
   {
      struct
      {
         int32_t first;
         int32_t last;
      } interval;
      struct
      {
         int32_t left;
         int32_t right;
      } children;
   } index = {};
};

class SegmentTree
{
public:
   explicit SegmentTree(PathSegments& segments);

   void produce_intersections(int32_t segment_index);

private:
   TreeNode build_tree(int32_t first, int32_t last, int32_t depth, const RectF& bounds);
   void produce_intersections_leaf(const TreeNode& node, int32_t segment_index);
   void
   produce_intersections(const TreeNode& node, int32_t segment_index, const RectF& segment_bounds, const RectF& node_bounds, int32_t axis);
   void intersect_lines(const LineF& line_a, const LineF& line_b, DataBuffer<SegmentIntersection>& intersections);

   PathSegments& _segments;
   std::vector<int32_t> _index;
   RectF _bounds;
   std::vector<TreeNode> _tree;
   DataBuffer<SegmentIntersection> _intersections;
};

SegmentTree::SegmentTree(PathSegments& segments) : _segments(segments), _intersections(0)
{
   _bounds = {
      std::numeric_limits<double>::infinity(),
      std::numeric_limits<double>::infinity(),
      -std::numeric_limits<double>::infinity(),
      -std::numeric_limits<double>::infinity()
   };

   _index.resize(_segments.segments());

   for (int32_t segment_index = 0; segment_index < static_cast<int32_t>(_index.size()); ++segment_index)
   {
      _index[segment_index] = segment_index;

      const RectF& seg_bounds = _segments.elementBounds(segment_index);

      if (seg_bounds.x1 < _bounds.x1)
      {
         _bounds.x1 = seg_bounds.x1;
      }
      if (seg_bounds.y1 < _bounds.y1)
      {
         _bounds.y1 = seg_bounds.y1;
      }
      if (seg_bounds.x2 > _bounds.x2)
      {
         _bounds.x2 = seg_bounds.x2;
      }
      if (seg_bounds.y2 > _bounds.y2)
      {
         _bounds.y2 = seg_bounds.y2;
      }
   }

   _tree.resize(1);
   _tree[0] = build_tree(0, static_cast<int32_t>(_index.size()), 0, _bounds);
}

TreeNode SegmentTree::build_tree(int32_t first, int32_t last, int32_t depth, const RectF& bounds)
{
   if (depth >= 24 || (last - first) <= 10)
   {
      TreeNode leaf_node = {};
      leaf_node.leaf = true;
      leaf_node.index.interval.first = first;
      leaf_node.index.interval.last = last;
      return leaf_node;
   }

   const auto split_axis = depth & int32_t{1};

   TreeNode node;
   node.leaf = false;

   const double split = 0.5 * (bounds.minCoord(split_axis) + bounds.maxCoord(split_axis));

   node.split_left = bounds.minCoord(split_axis);
   node.split_right = bounds.maxCoord(split_axis);
   node.lowest_left_index = INT_MAX;
   node.lowest_right_index = INT_MAX;

   const auto tree_size = static_cast<int32_t>(_tree.size());
   node.index.children.left = tree_size;
   node.index.children.right = tree_size + 1;

   _tree.resize(tree_size + 2);

   int32_t left_cursor = first;
   int32_t right_cursor = last - 1;

   while (left_cursor <= right_cursor)
   {
      const int32_t current_index = _index[left_cursor];
      const RectF& seg_bounds = _segments.elementBounds(current_index);

      const double low_coord = seg_bounds.minCoord(split_axis);
      const double center_coord = (seg_bounds.minCoord(split_axis) + seg_bounds.maxCoord(split_axis)) * 0.5;

      if (center_coord < split)
      {
         const double high_coord = seg_bounds.maxCoord(split_axis);
         if (high_coord > node.split_left)
         {
            node.split_left = high_coord;
         }
         if (current_index < node.lowest_left_index)
         {
            node.lowest_left_index = current_index;
         }
         ++left_cursor;
      }
      else
      {
         if (low_coord < node.split_right)
         {
            node.split_right = low_coord;
         }
         if (current_index < node.lowest_right_index)
         {
            node.lowest_right_index = current_index;
         }
         std::swap(_index[left_cursor], _index[right_cursor]);
         --right_cursor;
      }
   }

   RectF left_bounds = bounds;
   left_bounds.maxCoordRef(split_axis) = node.split_left;

   RectF right_bounds = bounds;
   right_bounds.minCoordRef(split_axis) = node.split_right;

   _tree[node.index.children.left] = build_tree(first, left_cursor, depth + 1, left_bounds);
   _tree[node.index.children.right] = build_tree(left_cursor, last, depth + 1, right_bounds);

   return node;
}

void SegmentTree::intersect_lines(const LineF& line_a, const LineF& line_b, DataBuffer<SegmentIntersection>& intersections)
{
   const PointF p1 = line_a.p1;
   const PointF p2 = line_a.p2;
   const PointF q1 = line_b.p1;
   const PointF q2 = line_b.p2;

   if (points_are_close(p1, p2) || points_are_close(q1, q2))
   {
      return;
   }

   const bool p1_equals_q1 = points_are_close(p1, q1);
   const bool p2_equals_q2 = points_are_close(p2, q2);
   if (p1_equals_q1 && p2_equals_q2)
   {
      return;
   }

   const bool p1_equals_q2 = points_are_close(p1, q2);
   const bool p2_equals_q1 = points_are_close(p2, q1);
   if (p1_equals_q2 && p2_equals_q1)
   {
      return;
   }

   const PointF p_delta{p2.x - p1.x, p2.y - p1.y};
   const PointF q_delta{q2.x - q1.x, q2.y - q1.y};
   const double par = p_delta.x * q_delta.y - p_delta.y * q_delta.x;

   if (fuzzy_is_null(par))
   {
      const PointF normal{-p_delta.y, p_delta.x};
      const PointF q1_minus_p1{q1.x - p1.x, q1.y - p1.y};

      if (fuzzy_is_null(dot(normal, q1_minus_p1)))
      {
         const double inv_dp = 1.0 / dot(p_delta, p_delta);
         const PointF q1_min_p1{q1.x - p1.x, q1.y - p1.y};
         const PointF q2_min_p1{q2.x - p1.x, q2.y - p1.y};

         const double tq1 = dot(p_delta, q1_min_p1) * inv_dp;
         const double tq2 = dot(p_delta, q2_min_p1) * inv_dp;

         if (tq1 > 0.0 && tq1 < 1.0)
         {
            SegmentIntersection isect;
            isect.alpha_a = tq1;
            isect.alpha_b = 0.0;
            isect.pos = q1;
            intersections.add(isect);
         }
         if (tq2 > 0.0 && tq2 < 1.0)
         {
            SegmentIntersection isect;
            isect.alpha_a = tq2;
            isect.alpha_b = 1.0;
            isect.pos = q2;
            intersections.add(isect);
         }

         const double inv_dq = 1.0 / dot(q_delta, q_delta);
         const PointF p1_min_q1{p1.x - q1.x, p1.y - q1.y};
         const PointF p2_min_q1{p2.x - q1.x, p2.y - q1.y};

         const double tp1 = dot(q_delta, p1_min_q1) * inv_dq;
         const double tp2 = dot(q_delta, p2_min_q1) * inv_dq;

         if (tp1 > 0.0 && tp1 < 1.0)
         {
            SegmentIntersection isect;
            isect.alpha_a = 0.0;
            isect.alpha_b = tp1;
            isect.pos = p1;
            intersections.add(isect);
         }
         if (tp2 > 0.0 && tp2 < 1.0)
         {
            SegmentIntersection isect;
            isect.alpha_a = 1.0;
            isect.alpha_b = tp2;
            isect.pos = p2;
            intersections.add(isect);
         }
      }
      return;
   }

   if (p1_equals_q1 || p1_equals_q2 || p2_equals_q1 || p2_equals_q2)
   {
      return;
   }

   const double tp = (q_delta.y * (q1.x - p1.x) - q_delta.x * (q1.y - p1.y)) / par;
   const double tq = (p_delta.y * (q1.x - p1.x) - p_delta.x * (q1.y - p1.y)) / par;

   if (tp < 0.0 || tp > 1.0 || tq < 0.0 || tq > 1.0)
   {
      return;
   }

   const bool p_at_zero = fuzzy_is_null(tp);
   const bool p_at_one = fuzzy_is_null(tp - 1.0);
   const bool q_at_zero = fuzzy_is_null(tq);
   const bool q_at_one = fuzzy_is_null(tq - 1.0);

   if ((q_at_zero || q_at_one) && (p_at_zero || p_at_one))
   {
      return;
   }

   PointF intersection_point;
   if (p_at_zero)
   {
      intersection_point = p1;
   }
   else if (p_at_one)
   {
      intersection_point = p2;
   }
   else if (q_at_zero)
   {
      intersection_point = q1;
   }
   else if (q_at_one)
   {
      intersection_point = q2;
   }
   else
   {
      intersection_point = {q1.x + (q2.x - q1.x) * tq, q1.y + (q2.y - q1.y) * tq};
   }

   SegmentIntersection isect;
   isect.alpha_a = tp;
   isect.alpha_b = tq;
   isect.pos = intersection_point;
   intersections.add(isect);
}

void SegmentTree::produce_intersections_leaf(const TreeNode& node, int32_t segment_index)
{
   const RectF& bounds_a = _segments.elementBounds(segment_index);
   const LineF line_a = _segments.lineAt(segment_index);

   for (int32_t index_slot = node.index.interval.first; index_slot < node.index.interval.last; ++index_slot)
   {
      const int32_t other_index = _index[index_slot];
      if (other_index >= segment_index)
      {
         continue;
      }

      const RectF& bounds_b = _segments.elementBounds(other_index);

      if (bounds_a.x1 > bounds_b.x2 || bounds_b.x1 > bounds_a.x2)
      {
         continue;
      }
      if (bounds_a.y1 > bounds_b.y2 || bounds_b.y1 > bounds_a.y2)
      {
         continue;
      }

      _intersections.reset();

      const LineF line_b = _segments.lineAt(other_index);
      intersect_lines(line_a, line_b, _intersections);

      for (int32_t intersection_index = 0; intersection_index < _intersections.size(); ++intersection_index)
      {
         const SegmentIntersection& seg_isect = _intersections.at(intersection_index);

         PathSegments::Intersection isect_a;
         isect_a.t = seg_isect.alpha_a;
         isect_a.next = 0;

         PathSegments::Intersection isect_b;
         isect_b.t = seg_isect.alpha_b;
         isect_b.next = 0;

         isect_a.vertex = isect_b.vertex = _segments.addPoint(seg_isect.pos);

         _segments.addIntersection(segment_index, isect_a);
         _segments.addIntersection(other_index, isect_b);
      }
   }
}

void SegmentTree::produce_intersections(
   const TreeNode& node,
   int32_t segment_index,
   const RectF& segment_bounds,
   const RectF& node_bounds,
   int32_t axis
)
{
   if (node.leaf)
   {
      produce_intersections_leaf(node, segment_index);
      return;
   }

   RectF left_bounds = node_bounds;
   left_bounds.maxCoordRef(axis) = node.split_left;

   RectF right_bounds = node_bounds;
   right_bounds.minCoordRef(axis) = node.split_right;

   if (segment_index > node.lowest_left_index && segment_bounds.minCoord(axis) <= node.split_left)
   {
      produce_intersections(_tree[node.index.children.left], segment_index, segment_bounds, left_bounds, !axis);
   }
   if (segment_index > node.lowest_right_index && segment_bounds.maxCoord(axis) >= node.split_right)
   {
      produce_intersections(_tree[node.index.children.right], segment_index, segment_bounds, right_bounds, !axis);
   }
}

void SegmentTree::produce_intersections(int32_t segment_index)
{
   const RectF& seg_bounds = _segments.elementBounds(segment_index);
   produce_intersections(_tree[0], segment_index, seg_bounds, _bounds, 0);
}

// ---------------------------------------------------------------------------
// Intersection finder  — drives SegmentTree over all segments
// ---------------------------------------------------------------------------

class IntersectionFinder
{
public:
   void produce_intersections(PathSegments& segments)
   {
      SegmentTree tree(segments);
      for (int32_t segment_index = 0; segment_index < segments.segments(); ++segment_index)
      {
         tree.produce_intersections(segment_index);
      }
   }
};

// ---------------------------------------------------------------------------
// Angle helper  — maps a direction vector to a 0-128 monotone angle scale
// ---------------------------------------------------------------------------

double compute_angle(const PointF& direction)
{
   if (direction.x == 0.0)
   {
      return direction.y <= 0.0 ? 0.0 : 64.0;
   }
   else if (direction.y == 0.0)
   {
      return direction.x <= 0.0 ? 32.0 : 96.0;
   }

   double dir_x = direction.x;
   double dir_y = direction.y;
   normalize_in_place(dir_x, dir_y);

   if (dir_y < 0.0)
   {
      if (dir_x < 0.0)
      {
         return -32.0 * dir_x;
      }
      else
      {
         return 128.0 - 32.0 * dir_x;
      }
   }
   else
   {
      return 64.0 + 32.0 * dir_x;
   }
}

// ---------------------------------------------------------------------------
// Graph helpers
// ---------------------------------------------------------------------------

int32_t common_edge(const WingedEdge& graph, int32_t vertex_a, int32_t vertex_b)
{
   const PathVertex* const vertex_ptr_a = graph.vertex(vertex_a);
   const PathVertex* const vertex_ptr_b = graph.vertex(vertex_b);

   if (!vertex_ptr_a || !vertex_ptr_b)
   {
      return -1;
   }
   if (vertex_ptr_a->edge < 0 || vertex_ptr_b->edge < 0)
   {
      return -1;
   }

   WingedEdge::TraversalStatus status;
   status.edge = vertex_ptr_a->edge;
   status.direction = graph.edge(status.edge)->directionTo(vertex_a);
   status.traversal = PathEdge::Traversal::Right;

   do
   {
      const PathEdge* const edge_ptr = graph.edge(status.edge);
      if ((edge_ptr->first == vertex_a && edge_ptr->second == vertex_b) || (edge_ptr->first == vertex_b && edge_ptr->second == vertex_a))
      {
         return status.edge;
      }
      status = graph.next(status);
      status.flip();
   } while (status.edge != vertex_ptr_a->edge);

   return -1;
}

void add_line_to(PainterPath& path, const PointF& target_point)
{
   const auto element_count = path.elementCount();
   if (element_count >= 2)
   {
      const PainterPath::Element& middle = path.elementAt(element_count - 1);
      if (middle.type == PainterPath::ElementType::LineTo)
      {
         const PainterPath::Element& first_elem = path.elementAt(element_count - 2);
         const PointF first_point{first_elem.x, first_elem.y};
         const PointF to_target{target_point.x - first_point.x, target_point.y - first_point.y};
         const PointF to_middle{middle.x - first_point.x, middle.y - first_point.y};
         const PointF perpendicular{-to_target.y, to_target.x};
         if (fuzzy_is_null(dot(perpendicular, to_middle)))
         {
            path.setElementPositionAt(element_count - 1, target_point.x, target_point.y);
            return;
         }
      }
   }
   path.lineTo(target_point.x, target_point.y);
}

void add_edge_to_path(PainterPath& path, const WingedEdge& graph, int32_t edge_index, PathEdge::Traversal traversal)
{
   WingedEdge::TraversalStatus status;
   status.edge = edge_index;
   status.traversal = traversal;
   status.direction = PathEdge::Direction::Forward;

   const PathVertex* const start_vertex = graph.vertex(graph.edge(edge_index)->first);
   path.moveTo(start_vertex->x, start_vertex->y);

   do
   {
      const PathEdge* const edge_ptr = graph.edge(status.edge);
      const PathVertex* const target_vertex = graph.vertex(edge_ptr->vertex(status.direction));
      add_line_to(path, {target_vertex->x, target_vertex->y});

      if (status.traversal == PathEdge::Traversal::Left)
      {
         edge_ptr->flag &= ~16;
      }
      else
      {
         edge_ptr->flag &= ~32;
      }

      status = graph.next(status);
   } while (status.edge != edge_index);
}

}  // namespace

// ---------------------------------------------------------------------------
// WingedEdge implementation
// ---------------------------------------------------------------------------

WingedEdge::WingedEdge() : _edges(0), _vertices(0), _segments(0)
{
}

WingedEdge::WingedEdge(const PainterPath& subject, const PainterPath& clip)
    : _edges(subject.elementCount()), _vertices(subject.elementCount()), _segments(subject.elementCount())
{
   _segments.setPath(subject);
   _segments.addPath(clip);
   intersectAndAdd();
}

WingedEdge::TraversalStatus WingedEdge::next(const TraversalStatus& status) const
{
   const PathEdge* const current_edge = edge(status.edge);
   assert(current_edge);

   TraversalStatus result;
   result.edge = current_edge->next(status.traversal, status.direction);
   result.traversal = status.traversal;
   result.direction = status.direction;

   const PathEdge* const result_edge = edge(result.edge);
   assert(result_edge);

   if (current_edge->vertex(status.direction) == result_edge->vertex(status.direction))
   {
      result.flip();
   }

   return result;
}

double WingedEdge::delta(int32_t vertex_index, int32_t edge_a, int32_t edge_b) const
{
   const PathEdge* const edge_ptr_a = edge(edge_a);
   const PathEdge* const edge_ptr_b = edge(edge_b);

   double angle_a = edge_ptr_a->angle;
   double angle_b = edge_ptr_b->angle;

   if (vertex_index == edge_ptr_a->second)
   {
      angle_a = edge_ptr_a->inv_angle;
   }
   if (vertex_index == edge_ptr_b->second)
   {
      angle_b = edge_ptr_b->inv_angle;
   }

   const double result = angle_b - angle_a;

   if (result >= 128.0)
   {
      return result - 128.0;
   }
   else if (result < 0.0)
   {
      return result + 128.0;
   }
   return result;
}

WingedEdge::TraversalStatus WingedEdge::findInsertStatus(int32_t vertex_index, int32_t edge_index) const
{
   const PathVertex* const vertex_ptr = vertex(vertex_index);

   assert(vertex_ptr);
   assert(edge_index >= 0);
   assert(vertex_ptr->edge >= 0);

   double smallest_delta = 128.0;
   int32_t best_edge = vertex_ptr->edge;

   TraversalStatus status;
   status.direction = edge(vertex_ptr->edge)->directionTo(vertex_index);
   status.traversal = PathEdge::Traversal::Right;
   status.edge = vertex_ptr->edge;

   do
   {
      status = next(status);
      status.flip();

      assert(edge(status.edge)->vertex(status.direction) == vertex_index);

      const double delta_value = delta(vertex_index, edge_index, status.edge);
      if (delta_value < smallest_delta)
      {
         best_edge = status.edge;
         smallest_delta = delta_value;
      }
   } while (status.edge != vertex_ptr->edge);

   status.traversal = PathEdge::Traversal::Left;
   status.direction = PathEdge::Direction::Forward;
   status.edge = best_edge;

   if (edge(status.edge)->vertex(status.direction) != vertex_index)
   {
      status.flip();
   }

   assert(edge(status.edge)->vertex(status.direction) == vertex_index);

   return status;
}

void WingedEdge::removeEdge(int32_t edge_index)
{
   PathEdge* const edge_ptr = edge(edge_index);

   TraversalStatus status;
   status.direction = PathEdge::Direction::Forward;
   status.traversal = PathEdge::Traversal::Right;
   status.edge = edge_index;

   TraversalStatus forward_right = next(status);
   forward_right.flipDirection();

   status.traversal = PathEdge::Traversal::Left;
   TraversalStatus forward_left = next(status);
   forward_left.flipDirection();

   status.direction = PathEdge::Direction::Backward;
   TraversalStatus backward_left = next(status);
   backward_left.flipDirection();

   status.traversal = PathEdge::Traversal::Right;
   TraversalStatus backward_right = next(status);
   backward_right.flipDirection();

   edge(forward_right.edge)->setNext(forward_right.traversal, forward_right.direction, forward_left.edge);
   edge(forward_left.edge)->setNext(forward_left.traversal, forward_left.direction, forward_right.edge);

   edge(backward_right.edge)->setNext(backward_right.traversal, backward_right.direction, backward_left.edge);
   edge(backward_left.edge)->setNext(backward_left.traversal, backward_left.direction, backward_right.edge);

   edge_ptr->setNext(PathEdge::Direction::Forward, edge_index);
   edge_ptr->setNext(PathEdge::Direction::Backward, edge_index);

   PathVertex* const tail_vertex = vertex(edge_ptr->first);
   PathVertex* const head_vertex = vertex(edge_ptr->second);

   tail_vertex->edge = backward_right.edge;
   head_vertex->edge = forward_right.edge;
}

int32_t WingedEdge::insert(const PathVertex& vertex_to_insert)
{
   if (!_vertices.isEmpty())
   {
      const PathVertex& last_vertex = _vertices.last();
      if (vertex_to_insert.x == last_vertex.x && vertex_to_insert.y == last_vertex.y)
      {
         return _vertices.size() - 1;
      }

      for (int32_t vertex_index = 0; vertex_index < _vertices.size(); ++vertex_index)
      {
         const PathVertex& existing_vertex = _vertices.at(vertex_index);
         if (fuzzy_compare(existing_vertex.x, vertex_to_insert.x) && fuzzy_compare(existing_vertex.y, vertex_to_insert.y))
         {
            return vertex_index;
         }
      }
   }

   _vertices << vertex_to_insert;
   return _vertices.size() - 1;
}

int32_t WingedEdge::addEdge(const PointF& point_a, const PointF& point_b)
{
   const auto first_index = insert(PathVertex(point_a));
   const auto second_index = insert(PathVertex(point_b));
   return addEdge(first_index, second_index);
}

int32_t WingedEdge::addEdge(int32_t first_vertex_index, int32_t second_vertex_index)
{
   if (first_vertex_index == second_vertex_index)
   {
      return -1;
   }

   const auto existing_edge = common_edge(*this, first_vertex_index, second_vertex_index);
   if (existing_edge >= 0)
   {
      return existing_edge;
   }

   _edges << PathEdge(first_vertex_index, second_vertex_index);

   const auto new_edge_index = _edges.size() - 1;

   PathVertex* const first_vertex_ptr = vertex(first_vertex_index);
   PathVertex* const second_vertex_ptr = vertex(second_vertex_index);
   PathEdge* const new_edge_ptr = edge(new_edge_index);

   const PointF tangent{second_vertex_ptr->x - first_vertex_ptr->x, second_vertex_ptr->y - first_vertex_ptr->y};
   new_edge_ptr->angle = compute_angle(tangent);
   new_edge_ptr->inv_angle = new_edge_ptr->angle + 64.0;
   if (new_edge_ptr->inv_angle >= 128.0)
   {
      new_edge_ptr->inv_angle -= 128.0;
   }

   PathVertex* const vertex_ptrs[2] = {first_vertex_ptr, second_vertex_ptr};
   const PathEdge::Direction side_directions[2] = {PathEdge::Direction::Backward, PathEdge::Direction::Forward};

   for (int32_t side_index = 0; side_index < 2; ++side_index)
   {
      PathVertex* const current_vertex_ptr = vertex_ptrs[side_index];
      const PathEdge::Direction current_dir = side_directions[side_index];

      if (current_vertex_ptr->edge < 0)
      {
         current_vertex_ptr->edge = new_edge_index;
         new_edge_ptr->setNext(current_dir, new_edge_index);
      }
      else
      {
         const auto current_vertex_index = new_edge_ptr->vertex(current_dir);
         assert(vertex(current_vertex_index) == vertex_ptrs[side_index]);

         TraversalStatus status_after = findInsertStatus(current_vertex_index, new_edge_index);
         PathEdge* const edge_after = edge(status_after.edge);

         TraversalStatus status_before = next(status_after);
         status_before.flipDirection();
         PathEdge* const edge_before = edge(status_before.edge);

         edge_after->setNext(status_after.traversal, status_after.direction, new_edge_index);
         edge_before->setNext(status_before.traversal, status_before.direction, new_edge_index);

         const auto after_edge_index = status_after.edge;
         const auto before_edge_index = status_before.edge;

         status_after = next(status_after);
         status_before = next(status_before);

         status_after.flipDirection();
         status_before.flipDirection();

         assert(status_after.edge == new_edge_index);
         assert(status_before.edge == new_edge_index);

         new_edge_ptr->setNext(status_after.traversal, status_after.direction, after_edge_index);
         new_edge_ptr->setNext(status_before.traversal, status_before.direction, before_edge_index);
      }
   }

   assert(new_edge_ptr->next(PathEdge::Traversal::Right, PathEdge::Direction::Forward) >= 0);
   assert(new_edge_ptr->next(PathEdge::Traversal::Right, PathEdge::Direction::Backward) >= 0);
   assert(new_edge_ptr->next(PathEdge::Traversal::Left, PathEdge::Direction::Forward) >= 0);
   assert(new_edge_ptr->next(PathEdge::Traversal::Left, PathEdge::Direction::Backward) >= 0);

   return new_edge_index;
}

void WingedEdge::intersectAndAdd()
{
   IntersectionFinder finder;
   finder.produce_intersections(_segments);

   _segments.mergePoints();

   for (int32_t point_index = 0; point_index < _segments.points(); ++point_index)
   {
      static_cast<void>(addVertex(_segments.pointAt(point_index)));
   }

   DataBuffer<PathSegments::Intersection> local_intersections(_segments.segments());

   for (int32_t segment_index = 0; segment_index < _segments.segments(); ++segment_index)
   {
      local_intersections.reset();

      const auto path_id = _segments.pathId(segment_index);

      const PathSegments::Intersection* isect = _segments.intersectionAt(segment_index);
      while (isect)
      {
         local_intersections << *isect;
         if (isect->next)
         {
            isect += isect->next;
         }
         else
         {
            isect = nullptr;
         }
      }

      std::ranges::sort(local_intersections);

      auto current_vertex = _segments.segmentAt(segment_index).va;
      const auto end_vertex = _segments.segmentAt(segment_index).vb;

      for (int32_t isect_index = 0; isect_index < local_intersections.size(); ++isect_index)
      {
         const PathSegments::Intersection& current_isect = local_intersections.at(isect_index);
         PathEdge* const new_edge_ptr = edge(addEdge(current_vertex, current_isect.vertex));
         if (new_edge_ptr)
         {
            const auto winding_dir =
               _segments.pointAt(current_vertex).y < _segments.pointAt(current_isect.vertex).y ? int32_t{1} : int32_t{-1};
            if (path_id == 0)
            {
               new_edge_ptr->winding_a += winding_dir;
            }
            else
            {
               new_edge_ptr->winding_b += winding_dir;
            }
         }
         current_vertex = current_isect.vertex;
      }

      PathEdge* const final_edge_ptr = edge(addEdge(current_vertex, end_vertex));
      if (final_edge_ptr)
      {
         const auto winding_dir = _segments.pointAt(current_vertex).y < _segments.pointAt(end_vertex).y ? int32_t{1} : int32_t{-1};
         if (path_id == 0)
         {
            final_edge_ptr->winding_a += winding_dir;
         }
         else
         {
            final_edge_ptr->winding_b += winding_dir;
         }
      }
   }
}

bool WingedEdge::isInside(double x, double y) const
{
   int32_t winding = 0;
   for (int32_t edge_index = 0; edge_index < edgeCount(); ++edge_index)
   {
      const PathEdge* const edge_ptr = edge(edge_index);

      const auto winding_weight = ((edge_ptr->flag >> 4) ^ (edge_ptr->flag >> 5)) & int32_t{1};
      if (!winding_weight)
      {
         continue;
      }

      const PathVertex* const vertex_a = vertex(edge_ptr->first);
      const PathVertex* const vertex_b = vertex(edge_ptr->second);

      if ((vertex_a->y < y && vertex_b->y > y) || (vertex_a->y > y && vertex_b->y < y))
      {
         const double intersection_x = vertex_a->x + (vertex_b->x - vertex_a->x) * (y - vertex_a->y) / (vertex_b->y - vertex_a->y);
         if (intersection_x > x)
         {
            winding += winding_weight;
         }
      }
   }
   return (winding & 1) != 0;
}

void WingedEdge::simplify()
{
   for (int32_t edge_index = 0; edge_index < edgeCount(); ++edge_index)
   {
      const PathEdge* const edge_ptr = edge(edge_index);
      const int32_t both_sides_flag = 0x3 << 4;
      if ((edge_ptr->flag & both_sides_flag) == both_sides_flag)
      {
         removeEdge(edge_index);
         edge_ptr->flag &= ~both_sides_flag;
      }
   }
}

PainterPath WingedEdge::toPath() const
{
   PainterPath result;

   for (int32_t edge_index = 0; edge_index < edgeCount(); ++edge_index)
   {
      const PathEdge* const edge_ptr = edge(edge_index);

      if (edge_ptr->flag & 16)
      {
         add_edge_to_path(result, *this, edge_index, PathEdge::Traversal::Left);
      }
      if (edge_ptr->flag & 32)
      {
         add_edge_to_path(result, *this, edge_index, PathEdge::Traversal::Right);
      }
   }

   return result;
}

}  // namespace PathMerge
