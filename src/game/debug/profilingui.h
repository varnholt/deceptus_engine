#pragma once

#ifdef DEVELOPMENT_MODE

#include "game/debug/mechanismsample.h"
#include "game/debug/rendersectionsample.h"

#include <SFML/Graphics.hpp>
#include <SFML/System/Time.hpp>
#include <array>
#include <memory>
#include <vector>

struct ProfilingUi
{
   static constexpr int32_t sample_count = 256;

   ProfilingUi();

   void processEvents();
   void draw();
   void close();
   bool isOpen() const;
   void recordFrame(sf::Time frame_time, sf::Time update_time, sf::Time draw_time);
   void recordWindowDisplay(sf::Time display_time);
   void updateMechanismTimings(std::vector<MechanismSample> timings);
   void updateRenderSectionTimings(std::vector<RenderSectionSample> timings);

   ///
   /// \brief Tells whether the level should time each mechanism separately this frame.
   ///
   /// Per-mechanism timing costs a name lookup per mechanism per frame, which shows up in the very
   /// numbers being measured. The log-based flavour therefore alternates it between reports so that
   /// every other report carries an undistorted frame time.
   ///
   bool isMechanismProfilingWanted() const;

   std::unique_ptr<sf::RenderWindow> _render_window;
   sf::Clock _clock;

private:
   //! wall clock period between two frames. unlike _frame_times_ms this includes whatever the loop
   //! spends outside update and draw, i.e. the vsync wait, so it is the only one of these that
   //! yields the frame rate actually reached
   std::array<float, sample_count> _wall_times_ms{};
   std::array<float, sample_count> _frame_times_ms{};
   std::array<float, sample_count> _update_times_ms{};
   std::array<float, sample_count> _draw_times_ms{};
   std::array<float, sample_count> _window_display_times_ms{};
   std::array<float, sample_count> _tilemap_draw_calls{};       //!< tile map draw calls issued in that frame
   std::array<float, sample_count> _tilemap_target_switches{};  //!< render target changes between those draws
   std::array<float, sample_count> _layer_scan_steps{};         //!< candidates the z loop examined that frame
   std::array<float, sample_count> _tilemap_pixels_submitted{};
   std::array<float, sample_count> _tilemap_pixels_opaque{
   };  //!< of those, the fully opaque share  //!< tile pixels submitted that frame, for the overdraw factor
   sf::Clock _wall_clock;
   bool _wall_clock_primed{false};
   int32_t _write_index{0};
   int32_t _samples_written{0};  //!< how many of the ring buffer slots carry a real sample yet
   std::vector<MechanismSample> _mechanism_timings;
   std::vector<RenderSectionSample> _render_section_timings;
   int32_t _render_section_frames{0};  //!< frames accumulated into _render_section_timings so far
   sf::Clock _mechanism_update_clock;
   sf::Clock _log_clock;                    //!< paces the reports the log-based flavour writes
   bool _mechanism_profiling_wanted{true};  //!< alternated after every report, see isMechanismProfilingWanted()
};

#endif  // DEVELOPMENT_MODE
