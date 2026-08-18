#include "leveltransitionhandler.h"

#include <algorithm>
#include <chrono>
#include <memory>

#include "framework/tools/callbackmap.h"
#include "framework/tools/log.h"
#include "game/constants.h"
#include "game/effects/fadetransitioneffect.h"
#include "game/effects/screentransition.h"
#include "game/level/levelregistry.h"
#include "game/level/levels.h"
#include "game/player/playerregistry.h"
#include "game/state/savestate.h"

namespace
{
std::unique_ptr<ScreenTransition> makeFadeOutFadeInLevelTransition()
{
   auto screen_transition = std::make_unique<ScreenTransition>();
   const sf::Color fade_color{0, 0, 0};
   auto fade_out = std::make_shared<FadeTransitionEffect>(fade_color);
   auto fade_in = std::make_shared<FadeTransitionEffect>(fade_color);
   fade_out->_direction = FadeTransitionEffect::Direction::FadeOut;
   fade_out->_speed = 1.0f;
   fade_in->_direction = FadeTransitionEffect::Direction::FadeIn;
   fade_in->_value = 1.0f;
   fade_in->_speed = 1.0f;
   screen_transition->_effect_1 = fade_out;
   screen_transition->_effect_2 = fade_in;
   screen_transition->_delay_between_effects_ms = std::chrono::milliseconds{250};
   // the fade in is started by the game loop once the target level has finished loading
   screen_transition->_autostart_effect_2 = false;
   screen_transition->startEffect1();
   return screen_transition;
}
}  // namespace

LevelTransitionHandler& LevelTransitionHandler::getInstance()
{
   static LevelTransitionHandler __instance;
   return __instance;
}

void LevelTransitionHandler::request(const std::string& level_description_filename, const std::optional<sf::Vector2f>& spawn_position_px)
{
   if (_pending_request.has_value())
   {
      return;
   }

   _pending_request = Request{._level_description_filename = level_description_filename, ._spawn_position_px = spawn_position_px};
}

void LevelTransitionHandler::update()
{
   if (!_pending_request.has_value())
   {
      return;
   }

   // while the player is dead the death fade owns the screen; keep the request pending, the level is
   // about to be reloaded and the player can walk into the transition again
   const auto player = PlayerRegistry::getFirst();
   if (player && player->isDead())
   {
      return;
   }

   const auto request = _pending_request.value();
   _pending_request.reset();

   const auto& target_level_filename = request._level_description_filename;
   const auto levels = Levels::readLevelItems();
   const auto level_it = std::find_if(
      levels.begin(),
      levels.end(),
      [&target_level_filename](const auto& level_item) { return level_item._level_name == target_level_filename; }
   );

   if (level_it == levels.end())
   {
      Log::Error() << "level transition target '" << target_level_filename << "' is not listed in levels.json";
      return;
   }

   const auto target_level_index = static_cast<int32_t>(std::distance(levels.begin(), level_it));
   const auto spawn_position_px = request._spawn_position_px;

   auto screen_transition = makeFadeOutFadeInLevelTransition();
   screen_transition->_callbacks_effect_1_ended.emplace_back(
      [target_level_index, spawn_position_px]()
      {
         // remember what the player did in the level that is left behind, so it is restored when coming back
         const auto level = LevelRegistry::getCurrent();
         if (level)
         {
            level->saveState();
         }

         // the level index has to be updated before writing, otherwise the file still points at the level
         // that is being left and a crash or quit before the next save resumes in the wrong level
         SaveState::getCurrent()._level_index = target_level_index;

         SaveState::serializeToFile();

         LevelTransitionHandler::getInstance()._spawn_position_px = spawn_position_px;

         CallbackMap::getInstance().call(static_cast<int32_t>(CallbackType::LoadLevel));
      }
   );
   screen_transition->_callbacks_effect_2_ended.emplace_back([]() { ScreenTransitionHandler::getInstance().pop(); });
   ScreenTransitionHandler::getInstance().push(std::move(screen_transition));
}

std::optional<sf::Vector2f> LevelTransitionHandler::takeSpawnPosition()
{
   if (!_spawn_position_px.has_value())
   {
      return std::nullopt;
   }

   const auto spawn_position_px = _spawn_position_px;
   _spawn_position_px.reset();
   return spawn_position_px;
}

void LevelTransitionHandler::reset()
{
   _pending_request.reset();
   _spawn_position_px.reset();
}
