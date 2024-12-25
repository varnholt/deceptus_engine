#include "logui.h"

#pragma warning(push, 0)
#include "imgui/imgui-SFML.h"
#include "imgui/imgui.h"
#pragma warning(pop)

#include <iostream>
#include <mutex>

namespace
{
std::mutex _mutex;
std::vector<LogUiBuffer::LogItem> _log_items;
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
}

LogUi::LogUi() : _render_window(std::make_unique<sf::RenderWindow>(sf::VideoMode(800, 800), "deceptus log viewer"))
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

   ImGui::Checkbox("auto scroll", &_auto_scroll);

   ImGui::Separator();

   ImGui::BeginChild("log_scrollbar", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

   const auto& logs = _log_items;
   for (const auto& item : logs)
   {
      ImGui::TextUnformatted(item._message.c_str());
   }

   // Scroll to bottom if auto-scroll is enabled
   if (_auto_scroll /*&& gLoggerBuffer.shouldScrollToBottom()*/)
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
