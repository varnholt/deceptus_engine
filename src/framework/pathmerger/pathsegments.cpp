#include "pathsegments.h"

#include "mathhelpers.h"
#include "painterpath.h"

#include <algorithm>
#include <cassert>

namespace PathMerge
{

namespace
{

inline bool points_are_close(const PointF& point_a, const PointF& point_b)
{
   return MathHelpers::fuzzyIsNull(point_a.x - point_b.x) && MathHelpers::fuzzyIsNull(point_a.y - point_b.y);
}

// ---------------------------------------------------------------------------
// KdPointTree  — 2D kd-tree over the point list for O(n log n) fuzzy merging
// ---------------------------------------------------------------------------

struct KdNode
{
   int point = -1;     //!< Index into the parent PathSegments point list.
   int merged_id = -1; //!< Assigned merge-group id, or -1 if not yet visited.
   KdNode* left = nullptr;
   KdNode* right = nullptr;
};

class KdPointTree
{
public:
   enum class Traversal
   {
      Both,
      Left,
      Right,
      None
   };

   explicit KdPointTree(const PathSegments& segments);

   int build(int begin, int end, int depth = 0);

   KdNode* rootNode() { return _root_index >= 0 ? &_nodes.at(_root_index) : nullptr; }

   int nextId() { return _next_id++; }

private:
   friend class KdPointFinder;

   const PathSegments& _segments;
   DataBuffer<KdNode> _nodes;
   int _root_index = -1;
   int _next_id = 0;
};

KdPointTree::KdPointTree(const PathSegments& segments)
   : _segments(segments)
   , _nodes(segments.points())
{
   _nodes.resize(segments.points());

   for (int node_index = 0; node_index < _nodes.size(); ++node_index)
   {
      _nodes.at(node_index).point = node_index;
      _nodes.at(node_index).merged_id = -1;
   }

   if (_nodes.size() > 0)
   {
      _root_index = build(0, _nodes.size());
   }
}

int KdPointTree::build(int begin, int end, int depth)
{
   assert(end > begin);

   const int split_axis = depth & 1;
   const PointF& pivot_point = _segments.pointAt(_nodes.at(begin).point);
   const double pivot = split_axis == 0 ? pivot_point.x : pivot_point.y;

   int first = begin + 1;
   int last = end - 1;

   while (first <= last)
   {
      const PointF& current_point = _segments.pointAt(_nodes.at(first).point);
      const double value = split_axis == 0 ? current_point.x : current_point.y;
      if (value < pivot)
      {
         ++first;
      }
      else
      {
         std::swap(_nodes.at(first), _nodes.at(last));
         --last;
      }
   }

   if (last != begin)
   {
      std::swap(_nodes.at(last), _nodes.at(begin));
   }

   if (last > begin)
   {
      _nodes.at(last).left = &_nodes.at(build(begin, last, depth + 1));
   }
   else
   {
      _nodes.at(last).left = nullptr;
   }

   if (last + 1 < end)
   {
      _nodes.at(last).right = &_nodes.at(build(last + 1, end, depth + 1));
   }
   else
   {
      _nodes.at(last).right = nullptr;
   }

   return last;
}

template <typename Functor>
void traverseKdTree(KdNode& node, Functor& functor, int depth = 0)
{
   KdPointTree::Traversal status = functor(node, depth);

   const bool go_left = (status == KdPointTree::Traversal::Both || status == KdPointTree::Traversal::Left);
   const bool go_right = (status == KdPointTree::Traversal::Both || status == KdPointTree::Traversal::Right);

   if (go_left && node.left)
   {
      traverseKdTree(*node.left, functor, depth + 1);
   }
   if (go_right && node.right)
   {
      traverseKdTree(*node.right, functor, depth + 1);
   }
}

// ---------------------------------------------------------------------------
// KdPointFinder  — functor for traverseKdTree that locates a matching point
// ---------------------------------------------------------------------------

class KdPointFinder
{
public:
   KdPointFinder(int target_point_index, const PathSegments& segments, KdPointTree& tree)
      : _result(-1)
      , _segments(segments)
      , _tree(tree)
   {
      const PointF& target = segments.pointAt(target_point_index);
      _target_components[0] = target.x;
      _target_components[1] = target.y;
   }

   KdPointTree::Traversal operator()(KdNode& node, int depth)
   {
      if (_result != -1)
      {
         return KdPointTree::Traversal::None;
      }

      const PointF& node_point = _segments.pointAt(node.point);
      const double pivot_components[2] = {node_point.x, node_point.y};

      const int primary_axis = depth & 1;
      const int secondary_axis = (depth + 1) & 1;

      if (MathHelpers::fuzzyIsNull(pivot_components[primary_axis] - _target_components[primary_axis]))
      {
         if (MathHelpers::fuzzyIsNull(pivot_components[secondary_axis] - _target_components[secondary_axis]))
         {
            if (node.merged_id < 0)
            {
               node.merged_id = _tree.nextId();
            }
            _result = node.merged_id;
            return KdPointTree::Traversal::None;
         }
         return KdPointTree::Traversal::Both;
      }
      else if (_target_components[primary_axis] < pivot_components[primary_axis])
      {
         return KdPointTree::Traversal::Left;
      }
      else
      {
         return KdPointTree::Traversal::Right;
      }
   }

   int result() const { return _result; }

private:
   double _target_components[2];
   int _result;
   const PathSegments& _segments;
   KdPointTree& _tree;
};

} // namespace

// ---------------------------------------------------------------------------
// PathSegments
// ---------------------------------------------------------------------------

PathSegments::PathSegments(int reserve_count)
   : _points(reserve_count)
   , _segments(reserve_count)
   , _intersections(reserve_count)
   , _path_id(0)
{
}

void PathSegments::setPath(const PainterPath& path)
{
   _points.reset();
   _intersections.reset();
   _segments.reset();
   _path_id = 0;
   addPath(path);
}

void PathSegments::addPath(const PainterPath& path)
{
   const int first_new_segment = _segments.size();

   bool has_move_to = false;
   int last_move_to_vertex = 0;
   int last_vertex = 0;

   for (int element_index = 0; element_index < path.elementCount(); ++element_index)
   {
      const PainterPath::Element& element = path.elementAt(element_index);

      const PointF current_point{element.x, element.y};
      int current_vertex = _points.size();

      if (element_index > 0 && points_are_close(_points.at(last_move_to_vertex), current_point))
      {
         current_vertex = last_move_to_vertex;
      }
      else
      {
         _points << current_point;
      }

      if (element.type == PainterPath::ElementType::MoveTo)
      {
         if (has_move_to && last_vertex != last_move_to_vertex
             && !points_are_close(_points.at(last_vertex), _points.at(last_move_to_vertex)))
         {
            _segments << Segment(_path_id, last_vertex, last_move_to_vertex);
         }
         has_move_to = true;
         last_vertex = last_move_to_vertex = current_vertex;
      }
      else
      {
         _segments << Segment(_path_id, last_vertex, current_vertex);
         last_vertex = current_vertex;
      }
   }

   if (has_move_to && last_vertex != last_move_to_vertex
       && !points_are_close(_points.at(last_vertex), _points.at(last_move_to_vertex)))
   {
      _segments << Segment(_path_id, last_vertex, last_move_to_vertex);
   }

   for (int segment_index = first_new_segment; segment_index < _segments.size(); ++segment_index)
   {
      const LineF line = lineAt(segment_index);
      double bx1 = line.p1.x;
      double by1 = line.p1.y;
      double bx2 = line.p2.x;
      double by2 = line.p2.y;

      if (bx2 < bx1)
      {
         std::swap(bx1, bx2);
      }
      if (by2 < by1)
      {
         std::swap(by1, by2);
      }

      _segments.at(segment_index).bounds = RectF{bx1, by1, bx2, by2};
   }

   ++_path_id;
}

void PathSegments::mergePoints()
{
   KdPointTree tree(*this);

   if (tree.rootNode())
   {
      DataBuffer<PointF> merged_points(points());
      DataBuffer<int> point_index_mapping(points());

      for (int point_index = 0; point_index < points(); ++point_index)
      {
         KdPointFinder finder(point_index, *this, tree);
         traverseKdTree(*tree.rootNode(), finder);

         assert(finder.result() != -1);

         if (finder.result() >= merged_points.size())
         {
            merged_points << _points.at(point_index);
         }

         point_index_mapping << finder.result();
      }

      for (int segment_index = 0; segment_index < _segments.size(); ++segment_index)
      {
         _segments.at(segment_index).va = point_index_mapping.at(_segments.at(segment_index).va);
         _segments.at(segment_index).vb = point_index_mapping.at(_segments.at(segment_index).vb);
      }

      for (int isect_index = 0; isect_index < _intersections.size(); ++isect_index)
      {
         _intersections.at(isect_index).vertex = point_index_mapping.at(_intersections.at(isect_index).vertex);
      }

      _points.swap(merged_points);
   }
}

}  // namespace PathMerge
