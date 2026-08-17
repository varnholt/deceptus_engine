#include "profilingui.h"

#ifdef DEVELOPMENT_MODE
#include "game/debug/drawcallcounter.h"
#endif

#if defined(DEVELOPMENT_MODE) && !defined(DECEPTUS_VRSFML)

#pragma warning(push, 0)
#include "imgui/imgui-SFML.h"
#include "imgui/imgui.h"
#pragma warning(pop)

#include <algorithm>
#include <numeric>
#include <sstream>

ProfilingUi::ProfilingUi() : _render_window(std::make_unique<sf::RenderWindow>(sf::VideoMode({900, 900}), "deceptus profiling"))
{
   if (!ImGui::SFML::Init(*_render_window.get()))
   {
      // imgui-sfml init failed; window will still open but rendering is a no-op
   }
}

void ProfilingUi::processEvents()
{
   while (const auto event = _render_window->pollEvent())
   {
      ImGui::SFML::ProcessEvent(*_render_window.get(), event.value());

      if (event->is<sf::Event::Closed>())
      {
#ifndef DECEPTUS_VRSFML
         _render_window->close();
#endif
      }
   }
}

namespace
{
void drawTimingGraph(const char* label, const float* values, int32_t count, int32_t offset, float target_ms)
{
   const auto buffer_sum = std::accumulate(values, values + count, 0.0f);
   const auto average_ms = buffer_sum / static_cast<float>(count);
   const auto min_ms = *std::min_element(values, values + count);
   const auto max_ms = *std::max_element(values, values + count);

   std::ostringstream overlay_text;
   overlay_text << "avg: " << static_cast<int32_t>(average_ms) << " ms";

   const auto graph_width = ImGui::GetContentRegionAvail().x;
   ImGui::PlotLines(
      label, values, count, offset, overlay_text.str().c_str(), 0.0f, std::max(target_ms * 2.0f, max_ms * 1.1f), ImVec2(graph_width, 80.0f)
   );
   ImGui::Text("  min: %.2f ms   avg: %.2f ms   max: %.2f ms   target: %.2f ms", min_ms, average_ms, max_ms, target_ms);
}
}  // namespace

void ProfilingUi::draw()
{
   if (!_render_window->isOpen())
   {
      return;
   }

   ImGui::SFML::Update(*_render_window.get(), _clock.restart());

   const auto* viewport = ImGui::GetMainViewport();
   ImGui::SetNextWindowPos(viewport->WorkPos);
   ImGui::SetNextWindowSize(viewport->WorkSize);
   ImGui::Begin(
      "profiling", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar
   );

   const auto last_index = (_write_index + sample_count - 1) % sample_count;
   const auto last_frame_ms = _frame_times_ms[last_index];
   const auto last_wall_ms = _wall_times_ms[last_index];

   // update plus draw leaves out whatever the loop waits for outside the two, so the rate is taken
   // from the wall clock period instead
   const auto current_fps = (last_wall_ms > 0.0f) ? 1000.0f / last_wall_ms : 0.0f;

   ImGui::Text(
      "fps: %.1f   wall: %.2f ms   frame: %.2f ms   update: %.2f ms   draw: %.2f ms   swap: %.2f ms",
      current_fps,
      last_wall_ms,
      last_frame_ms,
      _update_times_ms[last_index],
      _draw_times_ms[last_index],
      _window_display_times_ms[last_index]
   );
   ImGui::Separator();

   constexpr auto target_frame_ms = 16.667f;
   constexpr auto target_update_ms = 8.333f;
   constexpr auto target_draw_ms = 8.333f;
   constexpr auto target_swap_ms = 4.0f;

   ImGui::Text("wall clock frame period");
   drawTimingGraph("##wall", _wall_times_ms.data(), sample_count, _write_index, target_frame_ms);

   ImGui::Spacing();
   ImGui::Text("frame time (update + draw)");
   drawTimingGraph("##frame", _frame_times_ms.data(), sample_count, _write_index, target_frame_ms);

   ImGui::Spacing();
   ImGui::Text("update time");
   drawTimingGraph("##update", _update_times_ms.data(), sample_count, _write_index, target_update_ms);

   ImGui::Spacing();
   ImGui::Text("draw time (scene + swap)");
   drawTimingGraph("##draw", _draw_times_ms.data(), sample_count, _write_index, target_draw_ms);

   ImGui::Spacing();
   ImGui::Text("swap time (window->display)");
   drawTimingGraph("##swap", _window_display_times_ms.data(), sample_count, _write_index, target_swap_ms);

   if (!_render_section_timings.empty())
   {
      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Text("render sections   (cpu side submit cost, in draw order)");
      ImGui::Spacing();
      for (const auto& sample : _render_section_timings)
      {
         ImGui::Text("%.3f ms  %s", sample.duration_ms, sample.name.c_str());
      }
   }

   if (!_mechanism_timings.empty())
   {
      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Text("mechanisms   (blue = update   orange = draw)");
      ImGui::Spacing();

      float max_total_ms = 0.0f;
      for (const auto& sample : _mechanism_timings)
      {
         max_total_ms = std::max(max_total_ms, sample.update_ms + sample.draw_ms);
      }
      if (max_total_ms < 0.001f)
      {
         max_total_ms = 0.001f;
      }

      const auto available_width = ImGui::GetContentRegionAvail().x;
      const auto max_bar_width = available_width * 0.55f;
      constexpr auto bar_height = 14.0f;
      constexpr auto bar_row_spacing = 2.0f;

      ImGui::BeginChild("##mechanism_scroll", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);
      ImDrawList* draw_list = ImGui::GetWindowDrawList();
      for (const auto& sample : _mechanism_timings)
      {
         const auto total_ms = sample.update_ms + sample.draw_ms;
         const auto update_bar_width = (sample.update_ms / max_total_ms) * max_bar_width;
         const auto draw_bar_width = (sample.draw_ms / max_total_ms) * max_bar_width;

         const auto cursor_screen_pos = ImGui::GetCursorScreenPos();

         if (update_bar_width > 0.0f)
         {
            draw_list->AddRectFilled(
               cursor_screen_pos,
               ImVec2(cursor_screen_pos.x + update_bar_width, cursor_screen_pos.y + bar_height),
               IM_COL32(50, 130, 200, 255)
            );
         }
         if (draw_bar_width > 0.0f)
         {
            draw_list->AddRectFilled(
               ImVec2(cursor_screen_pos.x + update_bar_width, cursor_screen_pos.y),
               ImVec2(cursor_screen_pos.x + update_bar_width + draw_bar_width, cursor_screen_pos.y + bar_height),
               IM_COL32(230, 130, 30, 255)
            );
         }

         ImGui::Dummy(ImVec2(max_bar_width, bar_height));
         ImGui::SameLine(0.0f, 8.0f);
         ImGui::Text("%.3f ms  %s", total_ms, sample.name.c_str());
         ImGui::SetCursorPosY(ImGui::GetCursorPosY() + bar_row_spacing);
      }
      ImGui::EndChild();
   }

   ImGui::End();

   _render_window->clear();
   ImGui::SFML::Render(*_render_window.get());
   _render_window->display();
}

void ProfilingUi::close()
{
   ImGui::SFML::Shutdown(*_render_window.get());
}

bool ProfilingUi::isOpen() const
{
   return _render_window->isOpen();
}

void ProfilingUi::recordFrame(sf::Time frame_time, sf::Time update_time, sf::Time draw_time)
{
   const auto wall_time = _wall_clock.restart();
   _wall_times_ms[_write_index] = (_wall_clock_primed ? wall_time.asSeconds() : frame_time.asSeconds()) * 1000.0f;
   _wall_clock_primed = true;
   _frame_times_ms[_write_index] = frame_time.asSeconds() * 1000.0f;
   _update_times_ms[_write_index] = update_time.asSeconds() * 1000.0f;
   _draw_times_ms[_write_index] = draw_time.asSeconds() * 1000.0f;
   _tilemap_draw_calls[_write_index] = static_cast<float>(DrawCallCounter::tilemap_draw_calls);
   _tilemap_target_switches[_write_index] = static_cast<float>(DrawCallCounter::tilemap_target_switches);
   DrawCallCounter::tilemap_draw_calls = 0;
   _layer_scan_steps[_write_index] = static_cast<float>(DrawCallCounter::layer_scan_steps);
   _tilemap_pixels_submitted[_write_index] = static_cast<float>(DrawCallCounter::tilemap_pixels_submitted);
   _tilemap_pixels_opaque[_write_index] = static_cast<float>(DrawCallCounter::tilemap_pixels_opaque);
   DrawCallCounter::tilemap_target_switches = 0;
   DrawCallCounter::tilemap_last_target = nullptr;
   DrawCallCounter::layer_scan_steps = 0;
   DrawCallCounter::tilemap_pixels_submitted = 0;
   DrawCallCounter::tilemap_pixels_opaque = 0;
   _write_index = (_write_index + 1) % sample_count;
   _samples_written = std::min(_samples_written + 1, sample_count);
}

void ProfilingUi::recordWindowDisplay(sf::Time display_time)
{
   _window_display_times_ms[_write_index] = display_time.asSeconds() * 1000.0f;
}

void ProfilingUi::updateMechanismTimings(std::vector<MechanismSample> timings)
{
   if (_mechanism_update_clock.getElapsedTime().asSeconds() < 0.5f)
   {
      return;
   }
   _mechanism_timings = std::move(timings);
   _mechanism_update_clock.restart();
}

void ProfilingUi::updateRenderSectionTimings(std::vector<RenderSectionSample> timings)
{
   _render_section_timings = std::move(timings);
}

bool ProfilingUi::isMechanismProfilingWanted() const
{
   return true;
}

#elif defined(DEVELOPMENT_MODE)

// there is no imgui window on the VRSFML targets, so the very same samples are summarized into the
// log instead. on the switch that log is the only artefact a run on real hardware leaves behind
// (see doc/switch_build.md), which makes this the only way to learn whether a frame is spent in
// update, in draw or waiting for vsync.

#include "framework/tools/log.h"
#include "game/config/gameconfiguration.h"

#include <algorithm>
#include <iomanip>
#include <numeric>
#include <sstream>

namespace
{

//! seconds between two reports written to the log
constexpr auto report_interval_s = 5.0f;

struct TimingSummary
{
   float minimum_ms{0.0f};
   float average_ms{0.0f};
   float maximum_ms{0.0f};
};

TimingSummary summarizeSamples(const float* values, int32_t count)
{
   if (count <= 0)
   {
      return {};
   }

   const auto sum_ms = std::accumulate(values, values + count, 0.0f);
   return {*std::min_element(values, values + count), sum_ms / static_cast<float>(count), *std::max_element(values, values + count)};
}

std::string formatSummary(const char* label, const TimingSummary& summary)
{
   std::ostringstream out_stream;
   out_stream << std::fixed << std::setprecision(2) << label << " min " << summary.minimum_ms << " avg " << summary.average_ms << " max "
              << summary.maximum_ms;
   return out_stream.str();
}

}  // namespace

ProfilingUi::ProfilingUi() = default;

void ProfilingUi::processEvents()
{
}

void ProfilingUi::draw()
{
   if (_log_clock.getElapsedTime().asSeconds() < report_interval_s)
   {
      return;
   }
   _log_clock.restart();

   if (_samples_written == 0)
   {
      return;
   }

   const auto wall_summary = summarizeSamples(_wall_times_ms.data(), _samples_written);
   const auto frame_summary = summarizeSamples(_frame_times_ms.data(), _samples_written);
   const auto update_summary = summarizeSamples(_update_times_ms.data(), _samples_written);
   const auto draw_summary = summarizeSamples(_draw_times_ms.data(), _samples_written);
   const auto display_summary = summarizeSamples(_window_display_times_ms.data(), _samples_written);

   // derived from the wall clock rather than from update plus draw, so that a loop that waits for
   // vsync somewhere outside the two reports the rate that is really reached
   const auto average_fps = (wall_summary.average_ms > 0.0f) ? 1000.0f / wall_summary.average_ms : 0.0f;

   std::ostringstream report;
   report << std::fixed << std::setprecision(2) << "profiling: fps " << average_fps << " over " << _samples_written << " frames"
          << " | " << formatSummary("wall", wall_summary) << " | " << formatSummary("frame", frame_summary) << " | "
          << formatSummary("update", update_summary) << " | " << formatSummary("draw", draw_summary) << " | "
          << formatSummary("swap", display_summary) << " | mechanism timing: " << (_mechanism_profiling_wanted ? "on" : "off")
          << " | vsync: " << (GameConfiguration::getInstance()._vsync_enabled ? "on" : "off");
   Log::Info() << report.str();

   const auto draw_call_summary = summarizeSamples(_tilemap_draw_calls.data(), _samples_written);
   std::ostringstream draw_call_line;
   draw_call_line << std::fixed << std::setprecision(1) << "profiling: tilemap draw calls per frame "
                  << formatSummary("", draw_call_summary) << " | target switches "
                  << formatSummary("", summarizeSamples(_tilemap_target_switches.data(), _samples_written)) << " | layer scan steps "
                  << formatSummary("", summarizeSamples(_layer_scan_steps.data(), _samples_written));

   // tile pixels submitted against the view area: how many times the average on screen pixel is
   // written by tile geometry alone. a pure count, so it reads the same here as on hardware
   const auto view_area = GameConfiguration::getInstance()._view_width * GameConfiguration::getInstance()._view_height;
   const auto pixel_summary = summarizeSamples(_tilemap_pixels_submitted.data(), _samples_written);
   if (view_area > 0)
   {
      const auto opaque_summary = summarizeSamples(_tilemap_pixels_opaque.data(), _samples_written);
      draw_call_line << " | tile overdraw " << std::setprecision(2) << (pixel_summary.average_ms / static_cast<float>(view_area))
                     << "x | opaque " << (opaque_summary.average_ms / static_cast<float>(view_area)) << "x";
   }
   Log::Info() << draw_call_line.str();

   // one line rather than one per section: on the switch every log line is a write to the sd card,
   // so a report that grows with the number of sections puts real pressure on that path
   if (!_render_section_timings.empty())
   {
      const auto section_frames = std::max(_render_section_frames, 1);
      auto section_total_ms = 0.0f;

      std::ostringstream section_line;
      section_line << std::fixed << std::setprecision(3) << "profiling: sections over " << section_frames << " frames |";
      for (const auto& sample : _render_section_timings)
      {
         const auto average_ms = sample.duration_ms / static_cast<float>(section_frames);
         section_line << " " << sample.name << " " << average_ms << " |";

         // Level::draw reports its own passes, and "level draw" is the span that contains them, so
         // counting both would total the level's cost twice and make the remainder come out negative
         if (sample.name != "level draw")
         {
            section_total_ms += average_ms;
         }
      }

      // the sections tile the whole of Game::draw, so whatever is left is time the cpu spent inside
      // a gl call waiting for the gpu rather than submitting work. that distinction decides whether
      // the frame is worth attacking on the cpu side at all
      section_line << " TOTAL " << section_total_ms << " | measured draw " << draw_summary.average_ms << " | unaccounted "
                   << (draw_summary.average_ms - section_total_ms);
      Log::Info() << section_line.str();
   }

   for (const auto& sample : _mechanism_timings)
   {
      std::ostringstream mechanism_line;
      mechanism_line << std::fixed << std::setprecision(3) << "profiling: mechanism " << sample.name << " update " << sample.update_ms
                     << " ms draw " << sample.draw_ms << " ms";
      Log::Info() << mechanism_line.str();
   }

   // the next window measures the other way around, so consecutive reports show both the
   // undistorted frame cost and the per mechanism breakdown that is paid for with some overhead
   _mechanism_profiling_wanted = !_mechanism_profiling_wanted;
   _mechanism_timings.clear();
   _render_section_timings.clear();
   _render_section_frames = 0;

   _samples_written = 0;
   _write_index = 0;
}

void ProfilingUi::close()
{
}

bool ProfilingUi::isOpen() const
{
   // there is no window that could be closed, so the instance lives until it is explicitly dropped
   return true;
}

void ProfilingUi::recordFrame(sf::Time frame_time, sf::Time update_time, sf::Time draw_time)
{
   const auto wall_time = _wall_clock.restart();
   _wall_times_ms[_write_index] = (_wall_clock_primed ? wall_time.asSeconds() : frame_time.asSeconds()) * 1000.0f;
   _wall_clock_primed = true;
   _frame_times_ms[_write_index] = frame_time.asSeconds() * 1000.0f;
   _update_times_ms[_write_index] = update_time.asSeconds() * 1000.0f;
   _draw_times_ms[_write_index] = draw_time.asSeconds() * 1000.0f;
   _tilemap_draw_calls[_write_index] = static_cast<float>(DrawCallCounter::tilemap_draw_calls);
   _tilemap_target_switches[_write_index] = static_cast<float>(DrawCallCounter::tilemap_target_switches);
   DrawCallCounter::tilemap_draw_calls = 0;
   _layer_scan_steps[_write_index] = static_cast<float>(DrawCallCounter::layer_scan_steps);
   _tilemap_pixels_submitted[_write_index] = static_cast<float>(DrawCallCounter::tilemap_pixels_submitted);
   _tilemap_pixels_opaque[_write_index] = static_cast<float>(DrawCallCounter::tilemap_pixels_opaque);
   DrawCallCounter::tilemap_target_switches = 0;
   DrawCallCounter::tilemap_last_target = nullptr;
   DrawCallCounter::layer_scan_steps = 0;
   DrawCallCounter::tilemap_pixels_submitted = 0;
   DrawCallCounter::tilemap_pixels_opaque = 0;
   _write_index = (_write_index + 1) % sample_count;
   _samples_written = std::min(_samples_written + 1, sample_count);
}

void ProfilingUi::recordWindowDisplay(sf::Time display_time)
{
   _window_display_times_ms[_write_index] = display_time.asSeconds() * 1000.0f;
}

void ProfilingUi::updateMechanismTimings(std::vector<MechanismSample> timings)
{
   if (_mechanism_update_clock.getElapsedTime().asSeconds() < 0.5f)
   {
      return;
   }
   _mechanism_timings = std::move(timings);
   _mechanism_update_clock.restart();
}

void ProfilingUi::updateRenderSectionTimings(std::vector<RenderSectionSample> timings)
{
   if (timings.empty())
   {
      return;
   }

   // a single frame's sections are far too noisy to compare against a frame time that is averaged
   // over the whole window, so they are summed here and divided by the frame count on reporting.
   // the sections come back in a fixed order, which is what makes accumulating by index safe
   if (_render_section_timings.size() != timings.size())
   {
      _render_section_timings = std::move(timings);
      _render_section_frames = 1;
      return;
   }

   for (size_t section_index = 0; section_index < timings.size(); section_index++)
   {
      _render_section_timings[section_index].duration_ms += timings[section_index].duration_ms;
   }
   _render_section_frames++;
}

bool ProfilingUi::isMechanismProfilingWanted() const
{
   return _mechanism_profiling_wanted;
}

#endif  // DEVELOPMENT_MODE
