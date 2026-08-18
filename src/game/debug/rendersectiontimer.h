#pragma once

#ifdef DEVELOPMENT_MODE

#include "game/debug/rendersectionsample.h"

#include <chrono>
#include <vector>

/// \brief accumulates cpu side durations between named marks within one frame's draw.
///
/// Level::draw and Game::draw both carve their work into sections and feed the same report, so the
/// sum of every section can be held against the measured draw time. Whatever is left over is the
/// part of the frame that nothing accounts for, which is what tells a gpu stall apart from work
/// that simply is not instrumented yet.
class RenderSectionTimer
{
public:
   /// \brief drops the previous frame's samples and starts a new measurement.
   /// \param enabled when false the timer stays inert, so callers can gate on a profiling flag
   ///        without branching around every mark.
   void begin(bool enabled)
   {
      _samples.clear();
      _enabled = enabled;

      if (!_enabled)
      {
         return;
      }

      _mark = std::chrono::high_resolution_clock::now();
   }

   /// \brief closes the running section under the given name and opens the next one.
   /// \param name label the elapsed time is recorded under; must outlive the report.
   void mark(const char* name)
   {
      if (!_enabled)
      {
         return;
      }

      const auto now = std::chrono::high_resolution_clock::now();
      _samples.push_back({name, std::chrono::duration<float, std::milli>(now - _mark).count()});
      _mark = now;
   }

   /// \brief returns the sections recorded since the last begin(), in call order.
   const std::vector<RenderSectionSample>& samples() const
   {
      return _samples;
   }

private:
   std::vector<RenderSectionSample> _samples;             //!< one entry per mark, in call order
   std::chrono::high_resolution_clock::time_point _mark;  //!< when the currently open section started
   bool _enabled{false};                                  //!< whether begin() armed the timer
};

#endif  // DEVELOPMENT_MODE
