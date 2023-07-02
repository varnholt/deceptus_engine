#include "game.h"

#include "animationpool.h"
#include "audio.h"
#include "camerapanorama.h"
#include "debugdraw.h"
#include "displaymode.h"
#include "eventserializer.h"
#include "fadetransitioneffect.h"
#include "framework/joystick/gamecontroller.h"
#include "framework/tools/callbackmap.h"
#include "framework/tools/globalclock.h"
#include "framework/tools/log.h"
#include "framework/tools/timer.h"
#include "gameclock.h"
#include "gameconfiguration.h"
#include "gamecontactlistener.h"
#include "gamecontrollerdata.h"
#include "gamecontrollerintegration.h"
#include "gamestate.h"
#include "level.h"
#include "levels.h"
#include "luainterface.h"
#include "messagebox.h"
#include "physics/physicsconfiguration.h"
#include "player/player.h"
#include "player/playerinfo.h"
#include "projectilehitanimation.h"
#include "savestate.h"
#include "screentransition.h"
#include "weapon.h"

#include "menus/menuscreenmain.h"
#include "menus/menuscreenpause.h"
#include "menus/menuscreenvideo.h"

#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

#include <time.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

// override WinUser.h
#ifdef MessageBox
#undef MessageBox
#endif

#ifdef __linux__
#define setUniform setParameter
#endif

// screen concept
//
// +-----------+------------------------------------------------------------------------------+-----------+        -
// |...........|..............................................................................|...........|        |
// |...........|..............................................................................|...........|        |
// |...........|..............................................................................|...........|        |
// |...........|..............................................................................|...........|        |
// +-----------+---------------------------------------+--------------------------------------+-----------+  -     |
// |...........|                                       |                                      |...........|  |     |
// |...........|                                       |                                      |...........|  |     |
// |...........|                                       |                                      |...........|  |     |
// |...........|                                       |                                      |...........|  |     |
// |...........|                                       |                                      |...........|  |     |
// |...........|                                       |                                      |...........|  |     |
// |...........|                                       |                                      |...........|  |     |
// |...........|                                       |                                      |...........|  |     |
// |...........|                V I E W                |                                      |...........|  |     |
// |...........||--------------- 640px ---------------||                                      |...........|
// |...........+---------------------------------------+--------------------------------------+...........| 720px 1080px
// |...........|#######################################|  -                                   |...........|
// |...........|---------------------------------------|  |                                   |...........|  |     |
// |...........|                                       |  |                                   |...........|  |     |
// |...........|                                       |                                      |...........|  |     |
// |...........|       O /                        +----| 360px                                |...........|  |     |
// |...........|     :()                          |####|                                      |...........|  |     |
// |...........|      / \                         |####|  |                                   |...........|  |     |
// |...........|-----'---"+     +-----------------+####|  |                                   |...........|  |     |
// |...........|##########|     |######################|  |                                   |...........|  |     |
// |...........|##########|:...:|######################|  -                                   |...........|  |     |
// +-----------+---------------------------------------+--------------------------------------+-----------+  -     |
// |...........|..............................................................................|...........|        |
// |...........|..............................................................................|...........|        |
// |...........|..............................................................................|...........|        |
// |...........|..............................................................................|...........|        |
// |...........|..............................................................................|...........|        |
// +-----------+------------------------------------------------------------------------------+-----------+        -
//
//             |------------------------------------ 1280px ----------------------------------|
//                                         R E N D E R T E X T U R E
//
// |------------------------------------------------ 1920px ----------------------------------------------|
//                                                 W I N D O W
//
// window width:           1280px
// window height:           720px
//
// view width:              640px
// view height:             360px
//
// ratio width:            1280px / 640px = 2
// ratio height:            720px / 360px = 2
//
// render texture width:    640 * 2 = 1280px
// render texture height:   360 * 2 = 720px

//----------------------------------------------------------------------------------------------------------------------
void Game::initializeWindow()
{
   const auto& game_config = GameConfiguration::getInstance();

   // since stencil buffers are used, it is required to enable them explicitly
   sf::ContextSettings context_settings;
   context_settings.stencilBits = 8;

   if (_window)
   {
      _window->close();
      _window.reset();
   }

   // the window size is whatever the user sets up or whatever fullscreen resolution the user has
   _window = std::make_shared<sf::RenderWindow>(
      sf::VideoMode(static_cast<uint32_t>(game_config._video_mode_width), static_cast<uint32_t>(game_config._video_mode_height)),
      GAME_NAME,
      game_config._fullscreen ? sf::Style::Fullscreen : sf::Style::Default,
      context_settings
   );

   _window->setVerticalSyncEnabled(game_config._vsync_enabled);
   _window->setFramerateLimit(60);
   _window->setKeyRepeatEnabled(false);
   _window->setMouseCursorVisible(!game_config._fullscreen);

   // reset render textures if needed
   if (_window_render_texture)
   {
      _window_render_texture.reset();
   }

   // this the render texture size derived from the window dimensions. as opposed to the window
   // dimensions this one takes the view dimensions into regard and preserves an integer multiplier
   const auto ratio_width = game_config._video_mode_width / game_config._view_width;
   const auto ratio_height = game_config._video_mode_height / game_config._view_height;

   const auto size_ratio = std::min(ratio_width, ratio_height);

   int32_t texture_width = size_ratio * game_config._view_width;
   int32_t texture_height = size_ratio * game_config._view_height;

   Log::Info() << "video mode: " << game_config._video_mode_width << " x " << game_config._video_mode_height
               << ", view size: " << game_config._view_width << " x " << game_config._view_height << ", ratio: " << size_ratio;

   _render_texture_offset.x = static_cast<uint32_t>((game_config._video_mode_width - texture_width) / 2);
   _render_texture_offset.y = static_cast<uint32_t>((game_config._video_mode_height - texture_height) / 2);

   _window_render_texture = std::make_shared<sf::RenderTexture>();

   if (!_window_render_texture->create(static_cast<uint32_t>(texture_width), static_cast<uint32_t>(texture_height)))
   {
      Log::Fatal() << "failed to create texture: window render texture";
   }

   Log::Info() << "created window render texture: " << texture_width << " x " << texture_height;

   if (!_level)
   {
      Log::Warning() << "level not initialized";
   }
   else
   {
      _level->initializeTextures();
   }

   EventSerializer::getInstance().setCallback([this](const sf::Event& event) { processEvent(event); });
}

//----------------------------------------------------------------------------------------------------------------------
void Game::initializeController()
{
   auto& gji = GameControllerIntegration::getInstance();

   gji.addDeviceAddedCallback(
      [this](int32_t /*id*/)
      {
         const auto& gji = GameControllerIntegration::getInstance();
         gji.getController()->addButtonPressedCallback(SDL_CONTROLLER_BUTTON_START, [this]() { showPauseMenu(); });
      }
   );

   gji.initialize();
}

//----------------------------------------------------------------------------------------------------------------------
void Game::showMainMenu()
{
   Menu::getInstance()->show(Menu::MenuType::Main);
   GameState::getInstance().enqueuePause();
}

//----------------------------------------------------------------------------------------------------------------------
void Game::showPauseMenu()
{
   // while the game is loading, don't bother to open the pause screen
   if (!_level_loading_finished)
   {
      return;
   }

   // don't allow to pause during screen transitions
   // don't allow to pause when the inventory is open (game is already paused)
   if (DisplayMode::getInstance().isSet(Display::ScreenTransition) || DisplayMode::getInstance().isSet(Display::IngameMenu))
   {
      return;
   }

   if (Menu::getInstance()->getCurrentType() == Menu::MenuType::None)
   {
      Menu::getInstance()->show(Menu::MenuType::Pause);
      GameState::getInstance().enqueuePause();
      _audio_callback(GameAudio::SoundEffect::GameStatePause);
   }
}

//----------------------------------------------------------------------------------------------------------------------
void Game::loadLevel()
{
   _level_loading_finished = false;
   _level_loading_finished_previous = false;

   _level_loading_thread = std::async(
      std::launch::async,
      [this]()
      {
         _player->resetWorld();  // free the pointer that's shared with the player
         _level.reset();

         // load level
         const auto level_item = Levels::readLevelItem(SaveState::getCurrent()._level_index);
         _level = std::make_shared<Level>();
         _level->setDescriptionFilename(level_item._level_name);
         _level->initialize();
         _level->initializeTextures();

         // put the player in there
         _player->setWorld(_level->getWorld());
         _player->initializeLevel();

         // jump back to stored position, that's only for debugging purposes, not for checkpoints
         if (_restore_previous_position)
         {
            _restore_previous_position = false;
            _player->setBodyViaPixelPosition(_stored_position.x, _stored_position.y);
         }

         _player->updatePixelRect();

         Log::Info() << "level loading finished";

         _level_loading_finished = true;

         // before synchronizing the camera with the player position, the camera needs to know its room limitations
         _level->syncRoom();
         CameraSystem::getInstance().syncNow();

         GameClock::getInstance().reset();
      }
   );
}

//----------------------------------------------------------------------------------------------------------------------
void Game::nextLevel()
{
   SaveState::getCurrent()._level_index++;

   auto levels = Levels::readLevelItems();
   if (SaveState::getCurrent()._level_index == levels.size())
   {
      // this could show the end sequence or similar
      // DrawStates::_draw_test_scene = true;

      SaveState::getCurrent()._level_index = 0;
   }

   loadLevel();
}

//----------------------------------------------------------------------------------------------------------------------
Game::~Game()
{
   // _event_serializer.serialize();
}

//----------------------------------------------------------------------------------------------------------------------
void Game::initialize()
{
   initializeController();

   _player = std::make_shared<Player>();
   _player->initialize();

   _info_layer = std::make_unique<InfoLayer>();
   _ingame_menu = std::make_unique<InGameMenu>();
   _controller_overlay = std::make_unique<ControllerOverlay>();
   _test_scene = std::make_unique<ForestScene>();

   CallbackMap::getInstance().addCallback(static_cast<int32_t>(CallbackType::NextLevel), [this]() { nextLevel(); });

   Audio::getInstance();

   // initially the game should be in main menu and paused
   std::dynamic_pointer_cast<MenuScreenMain>(Menu::getInstance()->getMenuScreen(Menu::MenuType::Main))
      ->setExitCallback([this]() { _window->close(); });

   std::dynamic_pointer_cast<MenuScreenVideo>(Menu::getInstance()->getMenuScreen(Menu::MenuType::Video))
      ->setFullscreenCallback([this]() { toggleFullScreen(); });

   std::dynamic_pointer_cast<MenuScreenVideo>(Menu::getInstance()->getMenuScreen(Menu::MenuType::Video))
      ->setResolutionCallback([this](int32_t w, int32_t h) { changeResolution(w, h); });

   std::dynamic_pointer_cast<MenuScreenVideo>(Menu::getInstance()->getMenuScreen(Menu::MenuType::Video))
      ->setVSyncCallback(
         [this]()
         {
            initializeWindow();
            if (!_level)
            {
               return;
            }
            _level->createViews();
         }
      );

   initializeWindow();

   showMainMenu();

   Timer::add(
      std::chrono::milliseconds(1000), [this]() { updateWindowTitle(); }, Timer::Type::Repeated, Timer::Scope::UpdateAlways
   );

   GameState::getInstance().addCallback(
      [this](ExecutionMode current, ExecutionMode previous)
      {
         if (current == ExecutionMode::Paused && previous == ExecutionMode::Running)
         {
            _player->getControls()->setKeysPressed(0);
            CameraPanorama::getInstance().updateLookState(Look::Active, false);
         }
      }
   );

   // it is quite likely that after going into paused state different keys are
   // pressed compared to before. actually it's always going to happen. that results
   // in unpredictable player behavior, such as running into a 'random' direction.
   // this is why, after going into pause and back, the keyboard needs to be synced
   GameState::getInstance().addCallback(
      [this](ExecutionMode current, ExecutionMode previous)
      {
         if (current == ExecutionMode::Running && previous == ExecutionMode::Paused)
         {
            _player->getControls()->forceSync();
         }
      }
   );

   // registering generic callback functions for the game console
   Console::getInstance().registerCallback("/ra", "reload animations", [] { Player::getCurrent()->reloadAnimationPool(); });

   GameAudio::getInstance().initialize();
   _audio_callback = [](GameAudio::SoundEffect effect) { GameAudio::getInstance().play(effect); };
}

// frambuffers
// - the window render texture
//    - the level render texture
//       - the level background render texture
//    - info layer
//    - menus
//    - inventory
//    - message boxes

//----------------------------------------------------------------------------------------------------------------------
void Game::draw()
{
   _fps++;

   _window_render_texture->clear();
   _window->clear(sf::Color::Black);
   _window->pushGLStates();

   _window_render_texture->clear();

   if (_level_loading_finished)
   {
      _level->draw(_window_render_texture, _screenshot);
   }

   _screenshot = false;

   ScreenTransitionHandler::getInstance().draw(_window_render_texture);

   _info_layer->setLoading(!_level_loading_finished);
   _info_layer->draw(*_window_render_texture.get());

   if (DrawStates::_draw_debug_info)
   {
      _info_layer->drawDebugInfo(*_window_render_texture.get());
   }

   if (DrawStates::_draw_console)
   {
      _info_layer->drawConsole(*_window_render_texture.get());
   }

   if (DrawStates::_draw_camera_system)
   {
      DebugDraw::debugCameraSystem(*_window_render_texture.get());
   }

   if (DrawStates::_draw_controller_overlay)
   {
      _controller_overlay->draw(*_window_render_texture.get());
   }

   if (DisplayMode::getInstance().isSet(Display::IngameMenu))
   {
      _ingame_menu->draw(*_window_render_texture.get());
   }

   if (DrawStates::_draw_test_scene)
   {
      _test_scene->draw(*_window_render_texture.get());
   }

   Menu::getInstance()->draw(*_window_render_texture.get(), {sf::BlendAlpha});
   MessageBox::draw(*_window_render_texture.get());

   _window_render_texture->display();
   auto window_texture_sprite = sf::Sprite(_window_render_texture->getTexture());

   if (GameConfiguration::getInstance()._fullscreen)
   {
      // scale window texture up to available window size
      const auto scale_x = _window->getSize().x / static_cast<float>(_window_render_texture->getSize().x);
      const auto scale_y = _window->getSize().y / static_cast<float>(_window_render_texture->getSize().y);
      const auto scale_minimum = std::min(static_cast<int32_t>(scale_x), static_cast<int32_t>(scale_y));
      const auto dx = (scale_x - scale_minimum) * 0.5f;
      const auto dy = (scale_y - scale_minimum) * 0.5f;
      window_texture_sprite.setPosition(_window_render_texture->getSize().x * dx, _window_render_texture->getSize().y * dy);
      window_texture_sprite.scale(static_cast<float>(scale_minimum), static_cast<float>(scale_minimum));
   }
   else
   {
      window_texture_sprite.setPosition(static_cast<float>(_render_texture_offset.x), static_cast<float>(_render_texture_offset.y));
   }

   _window->draw(window_texture_sprite);
   _window->popGLStates();
   _window->display();

   if (_recording)
   {
      const auto image = window_texture_sprite.getTexture()->copyToImage();

      std::thread record(
         [this, image]()
         {
            std::ostringstream num;
            num << std::setfill('0') << std::setw(5) << _recording_counter++;
            image.saveToFile(num.str() + ".bmp");
         }
      );

      record.detach();
   }

   if (DrawStates::_draw_physics_config)
   {
      _physics_ui->draw();
   }

   if (DrawStates::_draw_camera_system)
   {
      _camera_ui->draw();
   }
}

//----------------------------------------------------------------------------------------------------------------------
void Game::updateGameController()
{
   auto& gji = GameControllerIntegration::getInstance();

   gji.update();

   if (gji.isControllerConnected())
   {
      gji.getController()->update();
      _player->getControls()->setJoystickInfo(gji.getController()->getInfo());
   }
}

//----------------------------------------------------------------------------------------------------------------------
void Game::updateGameControllerForGame()
{
   const auto& gji = GameControllerIntegration::getInstance();

   if (gji.isControllerConnected())
   {
      const auto& controller_info = gji.getController()->getInfo();
      _player->getControls()->setJoystickInfo(controller_info);
      GameControllerData::getInstance().setJoystickInfo(controller_info);
   }
}

//----------------------------------------------------------------------------------------------------------------------
void Game::updateWindowTitle()
{
   std::ostringstream out_stream;
   out_stream << GAME_NAME << " - " << _fps << "fps";
   _window->setTitle(out_stream.str());
   _fps = 0;
}

//----------------------------------------------------------------------------------------------------------------------
void Game::goToLastCheckpoint()
{
   SaveState::deserializeFromFile();
   _player->reset();
   loadLevel();
}

//----------------------------------------------------------------------------------------------------------------------
void Game::menuLoadRequest()
{
   // the code below is mostly identical to 'goToLastCheckpoint'
   // however, this does not deserialize the last game state; anyhow - duplication should be removed
   ScreenTransitionHandler::getInstance().clear();
   _player->reset();
   loadLevel();
}

//----------------------------------------------------------------------------------------------------------------------
std::unique_ptr<ScreenTransition> Game::makeFadeOutFadeIn()
{
   auto screen_transition = std::make_unique<ScreenTransition>();
   const sf::Color fade_color{60, 0, 0};
   auto fade_out = std::make_shared<FadeTransitionEffect>(fade_color);
   auto fade_in = std::make_shared<FadeTransitionEffect>(fade_color);
   fade_out->_direction = FadeTransitionEffect::Direction::FadeOut;
   fade_out->_speed = 1.0f;
   fade_in->_direction = FadeTransitionEffect::Direction::FadeIn;
   fade_in->_value = 1.0f;
   fade_in->_speed = 2.0f;
   screen_transition->_effect_1 = fade_out;
   screen_transition->_effect_2 = fade_in;
   screen_transition->_delay_between_effects_ms = std::chrono::milliseconds{500};
   screen_transition->_autostart_effect_2 = false;
   screen_transition->startEffect1();

   return screen_transition;
}

//----------------------------------------------------------------------------------------------------------------------
void Game::resetAfterDeath(const sf::Time& dt)
{
   // not 100% if the screen transitions should actually drive the
   // level loading and game workflow. it should rather be the other
   // way round. on the other hand this approach allows very simple
   // timing and the fading is very unlikely to fail anyway.
   if (_player->isDead())
   {
      _death_wait_time_ms += dt.asMilliseconds();

      if (_death_wait_time_ms > 1000)
      {
         if (!ScreenTransitionHandler::getInstance().active())
         {
            // fade out/in
            // do the actual level reset once the fade out has happened
            auto screen_transition = makeFadeOutFadeIn();
            screen_transition->_callbacks_effect_1_ended.push_back([this]() { goToLastCheckpoint(); });
            screen_transition->_callbacks_effect_2_ended.push_back([]() { ScreenTransitionHandler::getInstance().pop(); });
            ScreenTransitionHandler::getInstance().push(std::move(screen_transition));
         }
      }
   }

   if (_level_loading_finished && !_level_loading_finished_previous)
   {
      _level_loading_finished_previous = true;
      ScreenTransitionHandler::getInstance().startEffect2();
   }
}

//----------------------------------------------------------------------------------------------------------------------
void Game::updateGameState(const sf::Time& dt)
{
   // check if just died
   auto death_reason = _player->checkDead();
   if (!_player->isDead() && death_reason != DeathReason::Invalid)
   {
      _death_wait_time_ms = 0;
      switch (death_reason)
      {
         case DeathReason::Laser:
         {
            Log::Info() << "dead: player got lasered";
            break;
         }
         case DeathReason::TouchesDeadly:
         {
            Log::Info() << "dead: touched something deadly";
            break;
         }
         case DeathReason::TooFast:
         {
            Log::Info() << "dead: too fast";
            break;
         }
         case DeathReason::OutOfHealth:
         {
            Log::Info() << "dead: out of health";
            break;
         }
         case DeathReason::Smashed:
         {
            Log::Info() << "dead: player got smashed";
            break;
         }
         case DeathReason::Invalid:
         {
            break;
         }
      }

      _player->die();
   }

   // fade out when the player dies
   // when the level is faded out, then start reloading
   resetAfterDeath(dt);
}

//----------------------------------------------------------------------------------------------------------------------
void Game::update()
{
   const auto dt = _delta_clock.getElapsedTime();
   _delta_clock.restart();

   Timer::update(Timer::Scope::UpdateAlways);
   Audio::getInstance().updateMusic();

   // update screen transitions here
   ScreenTransitionHandler::getInstance().update(dt);

   // reload the level when the save state has been invalidated, that means when a state is selected from the menu
   if (SaveState::getCurrent()._load_level_requested)
   {
      SaveState::getCurrent()._load_level_requested = false;
      menuLoadRequest();
   }

   _info_layer->update(dt);

   if (GameState::getInstance().getMode() == ExecutionMode::Paused)
   {
      updateGameController();

      if (DisplayMode::getInstance().isSet(Display::IngameMenu))
      {
         _ingame_menu->update(dt);
      }
   }
   else if (GameState::getInstance().getMode() == ExecutionMode::Running)
   {
      Timer::update(Timer::Scope::UpdateIngame);

      if (_level_loading_finished)
      {
         updateGameController();
         updateGameControllerForGame();

         _level->update(dt);
         _player->update(dt);

         if (DrawStates::_draw_test_scene)
         {
            _test_scene->update(dt);
         }

         // this might trigger level-reloading, so this ought to be the last drawing call in the loop
         updateGameState(dt);
      }
   }

   GameState::getInstance().sync();
   DisplayMode::getInstance().sync();
}

//----------------------------------------------------------------------------------------------------------------------
int32_t Game::loop()
{
   while (_window->isOpen())
   {
      processEvents();
      update();
      draw();
   }

   return 0;
}

//----------------------------------------------------------------------------------------------------------------------
void Game::reset()
{
   _player->reset();
}

//----------------------------------------------------------------------------------------------------------------------
void Game::toggleFullScreen()
{
   GameConfiguration::getInstance()._fullscreen = !GameConfiguration::getInstance()._fullscreen;
   initializeWindow();
   _level->createViews();
}

//----------------------------------------------------------------------------------------------------------------------
void Game::changeResolution(int32_t w, int32_t h)
{
   GameConfiguration::getInstance()._video_mode_width = w;
   GameConfiguration::getInstance()._video_mode_height = h;
   GameConfiguration::getInstance().serializeToFile();

   initializeWindow();

   if (_level)
   {
      _level->createViews();
   }
}

//----------------------------------------------------------------------------------------------------------------------
void Game::takeScreenshot()
{
   _screenshot = true;
}

//----------------------------------------------------------------------------------------------------------------------
void Game::processEvent(const sf::Event& event)
{
   if (event.type == sf::Event::Closed)
   {
      _window->close();
   }
   else if (event.type == sf::Event::LostFocus)
   {
      if (GameConfiguration::getInstance()._pause_mode == GameConfiguration::PauseMode::AutomaticPause)
      {
         if (
            !DisplayMode::getInstance().isSet(Display::IngameMenu)  // the in-game menu is save to leave open when losing the window focus
            && !Console::getInstance().isActive()                   // while the console is open, don't disturb
         )
         {
            showPauseMenu();
         }
      }
      else
      {
         if (_player)
         {
            CameraPanorama::getInstance().updateLookState(Look::Active, false);
            _player->getControls()->setKeysPressed(0);
         }
      }
   }
   else if (event.type == sf::Event::GainedFocus)
   {
      if (_player)
      {
         _player->getControls()->forceSync();
      }
   }
   else if (event.type == sf::Event::KeyPressed)
   {
      if (MessageBox::keyboardKeyPressed(event.key.code))
      {
         // nom nom nom
         return;
      }

      // todo: process keyboard events in the console class, just like done in the message box
      if (!Console::getInstance().isActive())
      {
         if (Menu::getInstance()->isVisible())
         {
            Menu::getInstance()->keyboardKeyPressed(event.key.code);
            return;
         }
         else
         {
            _player->getControls()->keyboardKeyPressed(event.key.code);
         }
      }

      processKeyPressedEvents(event);
   }
   else if (event.type == sf::Event::KeyReleased)
   {
      if (Menu::getInstance()->isVisible())
      {
         Menu::getInstance()->keyboardKeyReleased(event.key.code);
         return;
      }
      else
      {
         _player->getControls()->keyboardKeyReleased(event.key.code);
      }

      processKeyReleasedEvents(event);
   }
   else if (event.type == sf::Event::TextEntered)
   {
      if (Console::getInstance().isActive())
      {
         const auto unicode = event.text.unicode;
         if (unicode > 0x1F && unicode < 0x80)
         {
            Console::getInstance().append(static_cast<char>(unicode));
         }
      }
   }
}

//----------------------------------------------------------------------------------------------------------------------
void Game::shutdown()
{
   if (_physics_ui)
   {
      _physics_ui->close();
   }

   if (_camera_ui)
   {
      _camera_ui->close();
   }

   std::exit(0);
}

//----------------------------------------------------------------------------------------------------------------------
void Game::processKeyPressedEvents(const sf::Event& event)
{
   if (Console::getInstance().isActive())
   {
      // these should be moved to the console itself
      if (event.key.code == sf::Keyboard::Return)
      {
         Console::getInstance().execute();
      }
      if (event.key.code == sf::Keyboard::F11)
      {
         DrawStates::_draw_console = !DrawStates::_draw_console;
         Console::getInstance().setActive(DrawStates::_draw_console);
      }
      else if (event.key.code == sf::Keyboard::Backspace)
      {
         Console::getInstance().chop();
      }
      else if (event.key.code == sf::Keyboard::Up)
      {
         Console::getInstance().previousCommand();
      }
      else if (event.key.code == sf::Keyboard::Down)
      {
         Console::getInstance().nextCommand();
      }

      return;
   }

   if (DisplayMode::getInstance().isSet(Display::IngameMenu))
   {
      _ingame_menu->processEvent(event);
      return;
   }

   CameraPanorama::getInstance().processKeyPressedEvents(event);

   switch (event.key.code)
   {
      case sf::Keyboard::Num0:
      {
         Audio::getInstance().playSample({"powerup.wav"});

         if (SaveState::getPlayerInfo()._extra_table._skills._skills & static_cast<int32_t>(Skill::SkillType::WallClimb))
         {
            SaveState::getPlayerInfo()._extra_table._skills._skills &= ~static_cast<int32_t>(Skill::SkillType::WallClimb);
         }
         else
         {
            SaveState::getPlayerInfo()._extra_table._skills._skills |= static_cast<int32_t>(Skill::SkillType::WallClimb);
         }
         break;
      }
      case sf::Keyboard::F1:
      {
         DisplayMode::getInstance().enqueueToggle(Display::Debug);
         break;
      }
      case sf::Keyboard::F2:
      {
         DrawStates::_draw_controller_overlay = !DrawStates::_draw_controller_overlay;
         break;
      }
      case sf::Keyboard::F3:
      {
         DrawStates::_draw_camera_system = !DrawStates::_draw_camera_system;

         if (DrawStates::_draw_camera_system && !_camera_ui)
         {
            _camera_ui = std::make_unique<CameraSystemConfigurationUi>();
         }
         else if (_camera_ui)
         {
            _camera_ui->close();
            _camera_ui.reset();
         }

         break;
      }
      case sf::Keyboard::F4:
      {
         if (event.key.alt)
         {
            shutdown();
         }
         DrawStates::_draw_debug_info = !DrawStates::_draw_debug_info;
         break;
      }
      case sf::Keyboard::F6:
      {
         DrawStates::_draw_test_scene = !DrawStates::_draw_test_scene;
         break;
      }
      case sf::Keyboard::F7:
      {
         DrawStates::_draw_physics_config = !DrawStates::_draw_physics_config;

         if (DrawStates::_draw_physics_config && !_physics_ui)
         {
            _physics_ui = std::make_unique<PhysicsConfigurationUi>();
         }
         else if (_physics_ui)
         {
            _physics_ui->close();
            _physics_ui.reset();
         }

         break;
      }
      case sf::Keyboard::F11:
      {
         DrawStates::_draw_console = !DrawStates::_draw_console;
         Console::getInstance().setActive(DrawStates::_draw_console);
         break;
      }
      case sf::Keyboard::F:
      {
         toggleFullScreen();
         break;
      }
      case sf::Keyboard::I:
      {
         _ingame_menu->open();
         break;
      }
      case sf::Keyboard::L:
      {
         if (_level_loading_finished)
         {
            _restore_previous_position = true;
            _stored_position = _player->getPixelPositionFloat();
            _player->reset();
            loadLevel();
         }
         break;
      }
      case sf::Keyboard::M:
      {
         _recording = !_recording;
         break;
      }
      case sf::Keyboard::N:
      {
         nextLevel();
         break;
      }
      case sf::Keyboard::P:
      case sf::Keyboard::Escape:
      {
         showPauseMenu();
         break;
      }
      case sf::Keyboard::Q:
      {
         shutdown();
         break;
      }
      case sf::Keyboard::R:
      {
         reset();
         break;
      }
      case sf::Keyboard::S:
      {
         takeScreenshot();
         break;
      }
      case sf::Keyboard::V:
      {
         _player->setVisible(!_player->getVisible());
         break;
      }
      case sf::Keyboard::Tab:
      {
         _ingame_menu->open();
         break;
      }
      case sf::Keyboard::PageUp:
      {
         Level::getCurrentLevel()->getLightSystem()->increaseAmbient(0.1f);
         break;
      }
      case sf::Keyboard::PageDown:
      {
         Level::getCurrentLevel()->getLightSystem()->decreaseAmbient(0.1f);
         break;
      }
      default:
      {
         break;
      }
   }
}

//----------------------------------------------------------------------------------------------------------------------
void Game::processKeyReleasedEvents(const sf::Event& event)
{
   CameraPanorama::getInstance().processKeyReleasedEvents(event);
}

//----------------------------------------------------------------------------------------------------------------------
void Game::processEvents()
{
   sf::Event event;
   while (_window->pollEvent(event))
   {
      processEvent(event);
      EventSerializer::getInstance().add(event);
   }

   if (DrawStates::_draw_physics_config)
   {
      _physics_ui->processEvents();
   }

   if (DrawStates::_draw_camera_system)
   {
      _camera_ui->processEvents();
   }
}
