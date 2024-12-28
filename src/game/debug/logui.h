#ifndef LOGUI_H
#define LOGUI_H

#include "SFML/Graphics.hpp"

#include <chrono>
#include <source_location>

#include "framework/tools/log.h"

namespace LogUiBuffer
{
using SysClockTimePoint = std::chrono::time_point<std::chrono::system_clock>;
void log(const SysClockTimePoint&, Log::Level, const std::string&, const std::source_location&);

struct LogItem
{
   SysClockTimePoint _timepoint;
   Log::Level _level;
   std::string _message;
   std::source_location _source_location;
};

}  // namespace LogUiBuffer

class LogUi
{
public:
   LogUi();

   void processEvents();
   void draw();
   void close();

   std::unique_ptr<sf::RenderWindow> _render_window;
   sf::Clock _clock;
};

#endif  // LOGUI_H
