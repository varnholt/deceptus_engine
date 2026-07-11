#ifndef LOGUI_H
#define LOGUI_H

#include "SFML/Graphics.hpp"

#include <chrono>
#include <source_location>

#include "framework/tools/log.h"

namespace LogUiBuffer
{
using SysClockTimePoint = std::chrono::time_point<std::chrono::system_clock>;

/// \brief appends one log entry to the thread-safe ui buffer and trims old entries.
/// \param time_point timestamp captured when the log message was emitted.
/// \param level severity level of the log message.
/// \param message log text to display in the viewer.
/// \param source_location call site information used to show file and line tags.
void log(const SysClockTimePoint&, Log::Level, const std::string&, const std::source_location&);

/// \brief stores one buffered log record shown by the log viewer window.
struct LogItem
{
   SysClockTimePoint _timepoint;
   Log::Level _level;
   std::string _message;
   std::source_location _source_location;
};

}  // namespace LogUiBuffer

#ifndef __EMSCRIPTEN__
/// \brief renders a dedicated imgui window that displays recent log messages with severity coloring.
class LogUi
{
public:
   /// \brief creates the sfml window and initializes imgui-sfml bindings.
   LogUi();

   /// \brief processes sfml window events and forwards them to imgui.
   void processEvents();

   /// \brief advances imgui state and draws buffered log lines into the viewer window.
   void draw();

   /// \brief shuts down imgui-sfml resources used by the log viewer.
   void close();

   std::unique_ptr<sf::RenderWindow> _render_window;
   sf::Clock _clock;
};
#endif  // !__EMSCRIPTEN__

#endif  // LOGUI_H
