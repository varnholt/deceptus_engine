#include "splashscreen.h"

#include "framework/image/splash_animation.h"
#include "framework/tools/log.h"
#include "framework/tools/scopeexit.h"
#include "game/audio/audio.h"

#include <cstdint>
#include <exception>
#include <utility>
#include <vector>

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
      const auto cues = animation.getSoundCues();
      if (cues.empty())
      {
         animation.play(window);
         return;
      }

      auto& audio = Audio::getInstance();
      for (const auto& cue : cues)
      {
         audio.addSample(cue._sample);
      }

      std::vector<std::pair<int32_t, uint32_t>> playbacks;
      playbacks.reserve(cues.size());
      const ScopeExit stop_sounds(
         [&]
         {
            for (const auto& [slot, generation] : playbacks)
            {
               if (audio.getPlaybackGeneration(slot) == generation)
               {
                  audio.stopSample(slot);
               }
            }
         }
      );
      animation.play(
         window,
         [&](const std::string& sample)
         {
            if (const auto slot = audio.playSample({sample}))
            {
               playbacks.emplace_back(*slot, audio.getPlaybackGeneration(*slot));
            }
         }
      );
   }
   catch (const std::exception& error)
   {
      Log::Warning() << "startup splash skipped: " << error.what();
   }
}
}  // namespace SplashScreen
