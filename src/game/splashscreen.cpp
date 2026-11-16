#include "splashscreen.h"

#include "framework/image/splash_animation.h"
#include "framework/tools/log.h"

#include <exception>

namespace SplashScreen
{
void show(sf::RenderWindow& window, const std::string& config_path)
{
   static bool shown = false;
   if (shown)
   {
      return;
   }
   shown = true;
   try
   {
      const SplashAnimation animation(config_path);
      animation.play(window);
   }
   catch (const std::exception& error)
   {
      Log::Warning() << "startup splash skipped: " << error.what();
   }
}
}  // namespace SplashScreen
