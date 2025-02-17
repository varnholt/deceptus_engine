#include "logui.h"

#pragma warning(push, 0)
#include "imgui/imgui-SFML.h"
#include "imgui/imgui.h"
#pragma warning(pop)

#include <ctime>
#include <deque>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <mutex>

namespace
{
std::mutex _mutex;
std::deque<LogUiBuffer::LogItem> _log_items;

ImVec4 getLogLevelColor(Log::Level level)
{
   switch (level)
   {
      case Log::Level::Info:
      {
         return ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
      }
      case Log::Level::Warning:
      {
         return ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
      }
      case Log::Level::Error:
      {
         return ImVec4(1.0f, 0.5f, 0.0f, 1.0f);
      }
      case Log::Level::Fatal:
      {
         return ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
      }
   }
}
}  // namespace

void LogUiBuffer::log(
   const SysClockTimePoint& time_point,
   Log::Level level,
   const std::string& message,
   const std::source_location& location
)
{
   std::lock_guard<std::mutex> guard(_mutex);
   _log_items.push_back(LogItem{time_point, level, message, location});

   constexpr auto log_items_max_size = 1000;
   while (_log_items.size() > log_items_max_size)
   {
      _log_items.pop_front();
   }
}

LogUi::LogUi() : _render_window(std::make_unique<sf::RenderWindow>(sf::VideoMode({1200, 800}), "deceptus log viewer"))
{
   if (!ImGui::SFML::Init(*_render_window.get()))
   {
      std::cout << "could not create render window" << std::endl;
   }
}

void LogUi::processEvents()
{
   sf::Event event;
   while (_render_window->pollEvent(event))
   {
      ImGui::SFML::ProcessEvent(*_render_window.get(), event);

      if (event.type == sf::Event::Closed)
      {
         _render_window->close();
      }
   }
}

void LogUi::draw()
{
   ImGui::SFML::Update(*_render_window.get(), _clock.restart());

   ImGui::Begin("log");

   ImGui::PushItemWidth(ImGui::GetWindowWidth());
   ImGuiTreeNodeFlags header_flags = ImGuiTreeNodeFlags_DefaultOpen;

   ImGui::BeginChild("log_scrollbar", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

   const auto& logs = _log_items;
   for (const auto& item : logs)
   {
      ImVec4 color = getLogLevelColor(item._level);
      ImGui::PushStyleColor(ImGuiCol_Text, color);

      const auto now_time = std::chrono::system_clock::to_time_t(item._timepoint);
      std::stringstream time_ss;
      time_ss << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S");
      const auto now_local = time_ss.str();

      std::stringstream log_ss;
      log_ss << now_local << " " << Log::parseSourceTag(item._source_location) << ": " << item._message;

      const auto log_string = log_ss.str();
      ImGui::TextUnformatted(log_string.c_str());

      ImGui::PopStyleColor();
   }

   // autoscroll
   if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() * 0.95f)
   {
      ImGui::SetScrollHereY(1.0f);
   }

   ImGui::EndChild();

   ImGui::End();

   _render_window->clear();
   ImGui::SFML::Render(*_render_window.get());
   _render_window->display();
}

void LogUi::close()
{
   ImGui::SFML::Shutdown();
}
