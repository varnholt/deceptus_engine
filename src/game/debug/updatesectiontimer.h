#pragma once

#ifdef DEVELOPMENT_MODE

#include "game/debug/rendersectionsample.h"

#include <chrono>
#include <string_view>
#include <vector>

/// \brief accumulates cpu side durations between named marks across the simulation steps of one frame.
///
/// The draw side can push one sample per mark and average by index, because Game::draw runs its
/// passes exactly once per frame. The update side cannot: the simulation is stepped a whole number
/// of times per frame, so at 48 fps a frame runs one step or two and the same section is entered
/// once or twice. Accumulating by name instead of by position is what lets a section keep its
/// identity across that, and it makes the reported figure the per frame cost rather than the per
/// step one - which is the number that has to fit in the frame budget.
///
/// Why by position does not work. Two consecutive frames at 48 fps, marks in call order:
///
///        frame N (1 step)              frame N+1 (2 steps)
///        ------------------            --------------------------------------
///     0  game systems                  game systems
///     1  game controller               game controller
///     2  physics step        <-.        physics step        \
///     3  mechanisms            |        mechanisms           }  step 1
///     4  lua nodes             |        lua nodes           /
///     5  player update         |        physics step        \
///     6  sprite positions      |        mechanisms           }  step 2
///                              |        lua nodes           /
///                              |        player update
///                              |        sprite positions
///                              |
///        index 6 means          `------ index 6 means "mechanisms", second time round
///        "sprite positions"
///
/// So the lists differ in length and the same index means a different phase. Adding frame N+1 into
/// frame N by index would file the second step's physics under "player update". By name, the two
/// "physics step" entries land in the same slot and the frame reports what it really paid: two steps
/// worth. beginPass() is what separates the steps - it restarts the clock at the top of each one so
/// the first mark of step 2 is not charged for whatever ran between the steps.
class UpdateSectionTimer
{
public:
   /// \brief drops the previous frame's durations and starts a new measurement.
   /// \param enabled when false the timer stays inert, so callers can gate on a profiling flag
   ///        without branching around every mark.
   void beginFrame(bool enabled)
   {
      _samples.clear();
      _enabled = enabled;

      if (!_enabled)
      {
         return;
      }

      _mark = std::chrono::high_resolution_clock::now();
   }

   /// \brief restarts the clock without dropping what the frame has accumulated so far.
   /// \note call this at the top of a section group that runs more than once per frame. Without it
   ///       the first mark of the second simulation step would be charged for everything that ran
   ///       between the two steps.
   void beginPass()
   {
      if (!_enabled)
      {
         return;
      }

      _mark = std::chrono::high_resolution_clock::now();
   }

   /// \brief closes the running section under the given name and opens the next one.
   /// \param name label the elapsed time is added to; a name seen again in the same frame
   ///        accumulates rather than producing a second entry.
   void mark(const char* name)
   {
      if (!_enabled)
      {
         return;
      }

      const auto now = std::chrono::high_resolution_clock::now();
      const auto elapsed_ms = std::chrono::duration<float, std::milli>(now - _mark).count();
      _mark = now;

      // the sections are entered in the same order every step, so this search hits on its first
      // comparison in the steady state. it is a search rather than a cursor so that a step cut
      // short - a lua script asking for a level change mid update - cannot shift every later
      // section into the wrong slot
      for (auto& sample : _samples)
      {
         if (std::string_view{sample.name} == std::string_view{name})
         {
            sample.duration_ms += elapsed_ms;
            return;
         }
      }

      _samples.push_back({name, elapsed_ms});
   }

   /// \brief returns the sections recorded since the last beginFrame(), in first-seen order.
   const std::vector<RenderSectionSample>& samples() const
   {
      return _samples;
   }

private:
   std::vector<RenderSectionSample> _samples;             //!< one entry per distinct name, in first-seen order
   std::chrono::high_resolution_clock::time_point _mark;  //!< when the currently open section started
   bool _enabled{false};                                  //!< whether beginFrame() armed the timer
};

#endif  // DEVELOPMENT_MODE
