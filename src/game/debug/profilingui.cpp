#include "profilingui.h"

#if defined(DEVELOPMENT_MODE) && !defined(__EMSCRIPTEN__)

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
#ifndef __EMSCRIPTEN__
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
   const auto current_fps = (last_frame_ms > 0.0f) ? 1000.0f / last_frame_ms : 0.0f;

   ImGui::Text(
      "fps: %.1f   frame: %.2f ms   update: %.2f ms   draw: %.2f ms   swap: %.2f ms",
      current_fps,
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

   ImGui::Text("frame time");
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
   _frame_times_ms[_write_index] = frame_time.asSeconds() * 1000.0f;
   _update_times_ms[_write_index] = update_time.asSeconds() * 1000.0f;
   _draw_times_ms[_write_index] = draw_time.asSeconds() * 1000.0f;
   _write_index = (_write_index + 1) % sample_count;
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

#elif defined(DEVELOPMENT_MODE)

ProfilingUi::ProfilingUi() = default;
void ProfilingUi::processEvents()
{
}
void ProfilingUi::draw()
{
}
void ProfilingUi::close()
{
}
bool ProfilingUi::isOpen() const
{
   return false;
}
void ProfilingUi::recordFrame(sf::Time, sf::Time, sf::Time)
{
}
void ProfilingUi::recordWindowDisplay(sf::Time)
{
}
void ProfilingUi::updateMechanismTimings(std::vector<MechanismSample>)
{
}

#endif  // DEVELOPMENT_MODE
