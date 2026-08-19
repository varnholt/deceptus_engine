#include "profilingui.h"

#ifdef DEVELOPMENT_MODE
#include "game/debug/drawcallcounter.h"
#endif

#if defined(DEVELOPMENT_MODE) && !defined(DECEPTUS_VRSFML)

#pragma warning(push, 0)
#include "imgui/imgui-SFML.h"
#include "imgui/imgui.h"
#pragma warning(pop)

#include "framework/tools/log.h"
#include "game/config/gameconfiguration.h"

#include <algorithm>
#include <iomanip>
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

constexpr auto section_report_interval_s = 5.0f;

// the imgui window is fine to look at but a benchmark cannot read it, so the same numbers go into
// the log as one line. that is what makes two desktop runs comparable to each other, and to the
// line the console build already writes
void logRenderSections(const std::vector<RenderSectionSample>& samples, int32_t frames)
{
   auto section_total_ms = 0.0f;

   std::ostringstream section_line;
   section_line << std::fixed << std::setprecision(3) << "profiling: sections over " << frames << " frames |";
   for (const auto& sample : samples)
   {
      const auto average_ms = sample.duration_ms / static_cast<float>(frames);
      section_line << " " << sample.name << " " << average_ms << " |";

      // Level::draw reports its own passes, and "level draw" is the span that contains them, so
      // counting both would total the level's cost twice
      if (sample.name != "level draw")
      {
         section_total_ms += average_ms;
      }
   }

   section_line << " TOTAL " << section_total_ms;
   Log::Info() << section_line.str();
}

// draw call counts carry over to other hardware unchanged, unlike a timing, so they belong next to
// the sections rather than only in the imgui window
void logDrawCounts(
   const float* draw_calls,
   const float* ambient_occlusion_draw_calls,
   const float* target_switches,
   const float* scan_steps,
   const float* tilemap_pixels,
   const float* ambient_occlusion_pixels,
   const float* image_layer_pixels,
   const float* tilemap_normal_pixels,
   int32_t count
)
{
   if (count <= 0)
   {
      return;
   }

   const auto average = [count](const float* values) { return std::accumulate(values, values + count, 0.0f) / static_cast<float>(count); };

   std::ostringstream counts_line;
   counts_line << std::fixed << std::setprecision(1) << "profiling: tilemap draw calls per frame " << average(draw_calls)
               << " | ao draw calls " << average(ambient_occlusion_draw_calls) << " | target switches " << average(target_switches)
               << " | layer scan steps " << average(scan_steps);

   // a pixel count is machine independent, so the overdraw split reads the same here as on the
   // console. that is what makes a fill cut measurable on a desktop that is not fill bound
   const auto view_area = static_cast<float>(GameConfiguration::getInstance()._view_width * GameConfiguration::getInstance()._view_height);
   if (view_area > 0.0f)
   {
      counts_line << std::setprecision(2) << " | overdraw tiles " << (average(tilemap_pixels) / view_area) << "x (normal pass "
                  << (average(tilemap_normal_pixels) / view_area) << "x), ao " << (average(ambient_occlusion_pixels) / view_area)
                  << "x, image layers " << (average(image_layer_pixels) / view_area) << "x, total "
                  << ((average(tilemap_pixels) + average(ambient_occlusion_pixels) + average(image_layer_pixels)) / view_area) << "x";
   }
   Log::Info() << counts_line.str();
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
      const auto section_frames = std::max(_render_section_frames, 1);
      for (const auto& sample : _render_section_timings)
      {
         ImGui::Text("%.3f ms  %s", sample.duration_ms / static_cast<float>(section_frames), sample.name.c_str());
      }

      if (_log_clock.getElapsedTime().asSeconds() >= section_report_interval_s)
      {
         _log_clock.restart();
         logRenderSections(_render_section_timings, section_frames);
         logDrawCounts(
            _tilemap_draw_calls.data(),
            _ambient_occlusion_draw_calls.data(),
            _tilemap_target_switches.data(),
            _layer_scan_steps.data(),
            _tilemap_pixels_submitted.data(),
            _ambient_occlusion_pixels_submitted.data(),
            _image_layer_pixels_submitted.data(),
            _tilemap_normal_pixels_submitted.data(),
            _samples_written
         );
         _render_section_timings.clear();
         _render_section_frames = 0;
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
   _ambient_occlusion_draw_calls[_write_index] = static_cast<float>(DrawCallCounter::ambient_occlusion_draw_calls);
   DrawCallCounter::ambient_occlusion_draw_calls = 0;
   _tilemap_target_switches[_write_index] = static_cast<float>(DrawCallCounter::tilemap_target_switches);
   DrawCallCounter::tilemap_draw_calls = 0;
   _layer_scan_steps[_write_index] = static_cast<float>(DrawCallCounter::layer_scan_steps);
   _tilemap_pixels_submitted[_write_index] = static_cast<float>(DrawCallCounter::tilemap_pixels_submitted);
   _ambient_occlusion_pixels_submitted[_write_index] = static_cast<float>(DrawCallCounter::ambient_occlusion_pixels_submitted);
   _tilemap_normal_pixels_submitted[_write_index] = static_cast<float>(DrawCallCounter::tilemap_normal_pixels_submitted);
   _image_layer_pixels_submitted[_write_index] = static_cast<float>(DrawCallCounter::image_layer_pixels_submitted);
   DrawCallCounter::tilemap_target_switches = 0;
   DrawCallCounter::tilemap_last_target = nullptr;
   DrawCallCounter::layer_scan_steps = 0;
   DrawCallCounter::tilemap_pixels_submitted = 0;
   DrawCallCounter::ambient_occlusion_pixels_submitted = 0;
   DrawCallCounter::tilemap_normal_pixels_submitted = 0;
   DrawCallCounter::image_layer_pixels_submitted = 0;
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
   // a single frame's sections are far too noisy to compare between two runs, so they are summed
   // here and divided by the frame count on reporting, the same way the log flavour does it
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
                  << formatSummary("", draw_call_summary) << " | ao draw calls "
                  << formatSummary("", summarizeSamples(_ambient_occlusion_draw_calls.data(), _samples_written)) << " | target switches "
                  << formatSummary("", summarizeSamples(_tilemap_target_switches.data(), _samples_written)) << " | layer scan steps "
                  << formatSummary("", summarizeSamples(_layer_scan_steps.data(), _samples_written));

   // tile pixels submitted against the view area. this is an upper bound rather than a measurement:
   // it sums the colour and the normal target, and the animated tiles are gathered around the
   // player block rather than the view, so both inflate it. use it to watch a change move the
   // number, not as an absolute - lab/tile_opacity/analyze_opacity.py computes the real figure
   // offline from the tilesets
   const auto view_area = GameConfiguration::getInstance()._view_width * GameConfiguration::getInstance()._view_height;
   const auto pixel_summary = summarizeSamples(_tilemap_pixels_submitted.data(), _samples_written);
   const auto ambient_occlusion_pixel_summary = summarizeSamples(_ambient_occlusion_pixels_submitted.data(), _samples_written);
   const auto tilemap_normal_pixel_summary = summarizeSamples(_tilemap_normal_pixels_submitted.data(), _samples_written);
   const auto image_layer_pixel_summary = summarizeSamples(_image_layer_pixels_submitted.data(), _samples_written);
   if (view_area > 0)
   {
      // one screen written once is 1.0x, so these add up to how many times the frame pays for the
      // average pixel. splitting them by source is what turns "the frame is fill bound" into a
      // sorted list of what to cut
      const auto view_area_f = static_cast<float>(view_area);
      draw_call_line << std::setprecision(2) << " | overdraw tiles " << (pixel_summary.average_ms / view_area_f) << "x (normal pass "
                     << (tilemap_normal_pixel_summary.average_ms / view_area_f) << "x), ao "
                     << (ambient_occlusion_pixel_summary.average_ms / view_area_f) << "x, image layers "
                     << (image_layer_pixel_summary.average_ms / view_area_f) << "x, total "
                     << ((pixel_summary.average_ms + ambient_occlusion_pixel_summary.average_ms + image_layer_pixel_summary.average_ms) /
                         view_area_f)
                     << "x";
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
   _ambient_occlusion_draw_calls[_write_index] = static_cast<float>(DrawCallCounter::ambient_occlusion_draw_calls);
   DrawCallCounter::ambient_occlusion_draw_calls = 0;
   _tilemap_target_switches[_write_index] = static_cast<float>(DrawCallCounter::tilemap_target_switches);
   DrawCallCounter::tilemap_draw_calls = 0;
   _layer_scan_steps[_write_index] = static_cast<float>(DrawCallCounter::layer_scan_steps);
   _tilemap_pixels_submitted[_write_index] = static_cast<float>(DrawCallCounter::tilemap_pixels_submitted);
   _ambient_occlusion_pixels_submitted[_write_index] = static_cast<float>(DrawCallCounter::ambient_occlusion_pixels_submitted);
   _tilemap_normal_pixels_submitted[_write_index] = static_cast<float>(DrawCallCounter::tilemap_normal_pixels_submitted);
   _image_layer_pixels_submitted[_write_index] = static_cast<float>(DrawCallCounter::image_layer_pixels_submitted);
   DrawCallCounter::tilemap_target_switches = 0;
   DrawCallCounter::tilemap_last_target = nullptr;
   DrawCallCounter::layer_scan_steps = 0;
   DrawCallCounter::tilemap_pixels_submitted = 0;
   DrawCallCounter::ambient_occlusion_pixels_submitted = 0;
   DrawCallCounter::tilemap_normal_pixels_submitted = 0;
   DrawCallCounter::image_layer_pixels_submitted = 0;
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
