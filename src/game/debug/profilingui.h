#pragma once

#ifdef DEVELOPMENT_MODE

#include "game/debug/mechanismsample.h"

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

   std::unique_ptr<sf::RenderWindow> _render_window;
   sf::Clock _clock;

private:
   std::array<float, sample_count> _frame_times_ms{};
   std::array<float, sample_count> _update_times_ms{};
   std::array<float, sample_count> _draw_times_ms{};
   std::array<float, sample_count> _window_display_times_ms{};
   int32_t _write_index{0};
   std::vector<MechanismSample> _mechanism_timings;
   sf::Clock _mechanism_update_clock;
};

#endif  // DEVELOPMENT_MODE
