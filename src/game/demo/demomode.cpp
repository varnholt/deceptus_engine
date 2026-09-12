#include "demomode.h"

#include "framework/joystick/gamecontroller.h"
#include "framework/tools/log.h"
#include "framework/tools/sfmlcompat.h"
#include "game/audio/musicfilenames.h"
#include "game/audio/musicplayer.h"
#include "game/controller/gamecontrollerintegration.h"
#include "game/io/eventserializer.h"
#include "game/player/playercontrols.h"
#include "game/player/playerregistry.h"
#include "game/state/displaymode.h"
#include "game/state/gamestate.h"

#include <algorithm>
#include <chrono>
#include <fstream>

namespace
{
//! serializer the recorded events were captured with and are replayed through
constexpr auto event_serializer_name = "global";

//! name given to the throwaway save slot a demo runs in, so nothing mistakes it for an empty one
constexpr auto demo_player_name = "demo";
}  // namespace

DemoMode& DemoMode::getInstance()
{
   static DemoMode instance;
   return instance;
}

void DemoMode::initialize(const std::filesystem::path& config_path)
{
   if (!std::filesystem::exists(config_path))
   {
      return;
   }

   try
   {
      std::ifstream input_stream(config_path);
      const auto config = nlohmann::json::parse(input_stream);

      if (config.find("idle_timeout_s") != config.end())
      {
         _idle_timeout = sf::seconds(config.at("idle_timeout_s").get<float>());
      }

      for (const auto& demo_json : config.at("demos"))
      {
         DemoItem demo_item;
         demo_item._recording_path = demo_json.at("recording").get<std::string>();
         demo_item._level_index = demo_json.at("level_index").get<int32_t>();

         if (demo_json.find("save_state") != demo_json.end())
         {
            demo_item._save_state = demo_json.at("save_state");
         }

         _demo_items.push_back(demo_item);
      }
   }
   catch (const std::exception& exception)
   {
      Log::Error() << exception.what();
   }
}

void DemoMode::update(const sf::Time& delta_time)
{
   // controller buttons never arrive as events, so they are polled here in both states: to end a
   // running demo, and to count as activity while the menu is being navigated with a pad
   updateControllerInput();

   if (!_active)
   {
      updateIdleTimer(delta_time);
      return;
   }

   // dying would send the level through goToLastCheckpoint, which reads the player's own save file
   // back over the slot the demo is running in. a demo that kills the player has run its course
   // anyway, so it ends here instead
   const auto& player = PlayerRegistry::getFirst();
   if (player && player->isDead())
   {
      stop();
      return;
   }

   const auto serializer = EventSerializer::getInstance(event_serializer_name);
   if (!serializer || !serializer->isPlaying())
   {
      stop();
   }
}

void DemoMode::updateIdleTimer(const sf::Time& delta_time)
{
   if (_demo_items.empty())
   {
      return;
   }

   if (GameState::getInstance().getMode() != ExecutionMode::NotRunning)
   {
      return;
   }

   if (!Menu::getInstance()->isVisible())
   {
      return;
   }

   // a message box or a running transition means the menu is waiting on the player, not idling
   if (DisplayMode::getInstance().isAnySet(Display::Modal, Display::ScreenTransition))
   {
      return;
   }

   _idle_time += delta_time;

   if (_idle_time >= _idle_timeout)
   {
      start();
   }
}

void DemoMode::updateControllerInput()
{
   const auto& controller_integration = GameControllerIntegration::getInstance();

   if (!controller_integration.isControllerConnected())
   {
      _controller_button_pressed_previously = false;
      return;
   }

   const auto& button_values = controller_integration.getController()->getInfo().getButtonValues();
   const auto button_pressed = std::ranges::any_of(button_values, [](bool pressed) { return pressed; });

   if (button_pressed && !_controller_button_pressed_previously)
   {
      notifyUserInput();
   }

   _controller_button_pressed_previously = button_pressed;
}

bool DemoMode::notifyUserInput()
{
   _idle_time = sfcompat::timeZero();

   if (!_active)
   {
      return false;
   }

   stop();
   return true;
}

void DemoMode::positionPlayerAtRecordedStart()
{
   if (!_active)
   {
      return;
   }

   const auto serializer = EventSerializer::getInstance(event_serializer_name);
   if (!serializer)
   {
      return;
   }

   // a recording is started mid-level, so the spot it was captured at travels inside it
   const auto& start_position_px = serializer->getStartPosition();
   if (!start_position_px.has_value())
   {
      return;
   }

   const auto& player = PlayerRegistry::getFirst();
   if (!player)
   {
      return;
   }

   player->setBodyViaPixelPosition(start_position_px->x, start_position_px->y);
}

bool DemoMode::isActive() const
{
   return _active;
}

bool DemoMode::isUserInput(const sf::Event& event)
{
   return event.is<sf::Event::KeyPressed>() || event.is<sf::Event::MouseButtonPressed>();
}

void DemoMode::start()
{
   _idle_time = sfcompat::timeZero();

   const auto serializer = EventSerializer::getInstance(event_serializer_name);
   if (!serializer)
   {
      return;
   }

   const auto& demo_item = _demo_items[_next_demo_index];
   _next_demo_index = (_next_demo_index + 1) % _demo_items.size();

   if (!std::filesystem::exists(demo_item._recording_path))
   {
      Log::Error() << "demo recording not found: " << demo_item._recording_path;
      return;
   }

   serializer->deserialize(demo_item._recording_path);

   _menu_type_before = Menu::getInstance()->getCurrentType();
   _save_state_backup = SaveState::getCurrent();
   _controller_button_pressed_previously = false;
   _active = true;

   DisplayMode::getInstance().enqueueSet(Display::Demo);

   auto& save_state = SaveState::getCurrent();
   save_state = demo_item._save_state.has_value() ? demo_item._save_state->get<SaveState>() : SaveState{};
   save_state._player_info._name = demo_player_name;
   save_state._level_index = demo_item._level_index;

   // this is the same handover the file select menu does when a game is started
   Menu::getInstance()->hide();
   GameState::getInstance().enqueueResume();
   save_state._load_level_requested = true;

   // the replay clock only advances while the level is being stepped, so starting playback here does
   // not spend the first seconds of the demo on the loading screen
   // Ignore: the spawn position went into the level load above, the player is already there
   serializer->play(EventSerializer::StartPosition::Ignore);
}

void DemoMode::stop()
{
   _active = false;
   _idle_time = sfcompat::timeZero();

   const auto serializer = EventSerializer::getInstance(event_serializer_name);
   if (serializer)
   {
      serializer->stopPlayback();
   }

   // a demo that is cut short leaves the keys it replayed pressed, and those would otherwise carry
   // into whatever game is started next
   const auto& player = PlayerRegistry::getFirst();
   if (player && player->getControls())
   {
      player->getControls()->setKeysPressed(0);
   }

   SaveState::getCurrent() = _save_state_backup;

   GameState::getInstance().enqueueStop();
   Menu::getInstance()->show(_menu_type_before);
   DisplayMode::getInstance().enqueueUnset(Display::Demo);

   MusicPlayer::getInstance().queueTrack(
      {.filename = MusicFilenames::getMenuMusic().string(),
       .transition = MusicPlayerTypes::TransitionType::Crossfade,
       .duration = std::chrono::milliseconds(1000),
       .post_action = MusicPlayerTypes::PostPlaybackAction::Loop}
   );
}
