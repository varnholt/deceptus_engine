#pragma once

#include <cstdint>

namespace PathMerge
{

class PainterPath;

/// \brief Computes boolean and simplification operations on PainterPaths
///        using a sweep-line winged-edge algorithm.
class PathClipper
{
public:
   /// \brief The boolean or simplification operation to perform.
   enum class Operation
   {
      BoolAnd,  //!< Intersection of two paths.
      BoolOr,   //!< Union of two paths.
      BoolSub,  //!< Difference of two paths.
      Simplify  //!< Self-union: removes all self-intersections.
   };

   PathClipper(const PainterPath& subject, const PainterPath& clip);

   PainterPath clip(Operation operation);

private:
   enum class ClipperMode
   {
      Clip,  //!< Full clip — build the result path.
      Check  //!< Early exit as soon as any result edge is found.
   };

   bool handleCrossingEdges(class WingedEdge& graph, double y, ClipperMode mode);
   bool doClip(class WingedEdge& graph, ClipperMode mode);

   const PainterPath& _subject_path;  //!< The path being clipped.
   const PainterPath& _clip_path;     //!< The clip region (empty for Simplify).
   Operation _operation = Operation::Simplify;

   int32_t _a_mask = 0;  //!< Winding mask for the subject path fill rule.
   int32_t _b_mask = 0;  //!< Winding mask for the clip path fill rule.
};

}  // namespace PathMerge
