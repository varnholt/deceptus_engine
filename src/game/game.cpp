#include "game.h"

#include "framework/joystick/gamecontroller.h"
#include "framework/tools/callbackmap.h"
#include "framework/tools/localization.h"
#include "framework/tools/log.h"
#include "framework/tools/sfmlcompat.h"
#include "framework/tools/timer.h"
#include "game/audio/audio.h"
#include "game/audio/musicplayer.h"
#include "game/camera/camerapanorama.h"
#include "game/camera/camerasystem.h"
#include "game/clock/gameclock.h"
#include "game/config/gameconfiguration.h"
#include "game/controller/gamecontrollerdata.h"
#include "game/controller/gamecontrollerintegration.h"
#include "game/debug/debugdraw.h"
#include "game/debug/debugdrawstates.h"
#include "game/debug/mechanismschemawriter.h"
#include "game/effects/fadetransitioneffect.h"
#include "game/effects/screentransition.h"
#include "game/event/eventdistributor.h"
#include "game/level/level.h"
#include "game/level/levelregistry.h"
#include "game/level/levels.h"
#include "game/level/leveltransitionhandler.h"
#include "game/player/inventoryconfig.h"
#include "game/player/player.h"
#include "game/player/playerregistry.h"
#include "game/shaders/postprocessing.h"
#include "game/state/displaymode.h"
#include "game/state/gamestate.h"
#include "game/state/savestate.h"
#include "opengl/render3d/menubackgroundscene.h"
#include "opengl/render3d/texturedobject.h"
#include "splashscreen.h"

#include "menus/menu.h"
#include "menus/menuscreenmain.h"
#include "menus/menuscreenvideo.h"

#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#ifdef __SWITCH__
#include <switch.h>
#endif

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

// override WinUser.h
#ifdef MessageBox
#undef MessageBox
#endif

#include "game/ui/messagebox.h"

#ifdef __linux__
#define setUniform setParameter
#endif

namespace
{
void showGpu()
{
   // get the GPU vendor and renderer strings
   const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
   const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));

   if (vendor && renderer)
   {
      Log::Info() << "GPU vendor: " << vendor;
      Log::Info() << "GPU renderer: " << renderer;
   }
   else
   {
      Log::Warning() << "failed to retrieve GPU information";
   }
}

#ifndef DECEPTUS_VRSFML
void showErrorMessage(const std::string& message)
{
   sf::RenderWindow window(sf::VideoMode({240, 80}), "Error", sf::Style::Titlebar | sf::Style::Close);

   const sf::Font& font = getFont();

   sf::Text text(font);
   text.setFont(font);
   text.setString(message);
   text.setCharacterSize(12);
   text.setFillColor(sf::Color::Black);
   text.setPosition({30.0f, 30.0f});

   while (window.isOpen())
   {
      while (const auto event = window.pollEvent())
      {
         if (event->is<sf::Event::Closed>())
         {
            window.close();
         }
      }

      window.clear(sf::Color::White);
      window.draw(text);
      window.display();
   }
}
#endif

std::unique_ptr<ScreenTransition> makeFadeOutFadeInDeath()
{
   auto screen_transition = std::make_unique<ScreenTransition>();
   const sf::Color fade_color{0, 0, 0};
   auto fade_out = std::make_shared<FadeTransitionEffect>(fade_color);
   auto fade_in = std::make_shared<FadeTransitionEffect>(fade_color);
   fade_out->_direction = FadeTransitionEffect::Direction::FadeOut;
   fade_out->_speed = 1.0f;
   fade_in->_direction = FadeTransitionEffect::Direction::FadeIn;
   fade_in->_value = 1.0f;
   fade_in->_speed = 0.5f;
   screen_transition->_effect_1 = fade_out;
   screen_transition->_effect_2 = fade_in;
   screen_transition->_delay_between_effects_ms = std::chrono::milliseconds{500};
   screen_transition->_autostart_effect_2 = false;
   screen_transition->startEffect1();
   return screen_transition;
}

std::unique_ptr<ScreenTransition> makeFadeInAfterLoadGame()
{
   auto screen_transition = std::make_unique<ScreenTransition>();
   const sf::Color fade_color{0, 0, 0};
   auto null_effect = std::make_shared<ScreenTransitionEffect>();
   auto fade_in = std::make_shared<FadeTransitionEffect>(fade_color);
   fade_in->_direction = FadeTransitionEffect::Direction::FadeIn;
   fade_in->_value = 1.0f;
   fade_in->_speed = 1.0f;
   screen_transition->_effect_1 = null_effect;
   screen_transition->_effect_2 = fade_in;
   screen_transition->_delay_between_effects_ms = std::chrono::milliseconds{0};
   screen_transition->_autostart_effect_2 = true;
   screen_transition->startEffect1();
   return screen_transition;
}

}  // namespace

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

void Game::initializeWindow()
{
   auto& game_config = GameConfiguration::getInstance();

   // since stencil buffers are used, it is required to enable them explicitly
   sf::ContextSettings context_settings;
   context_settings.stencilBits = 8;

   if (_window)
   {
#ifndef DECEPTUS_VRSFML
      _window->close();
#endif
      _window.reset();
   }

   // the window size is whatever the user sets up or whatever fullscreen resolution the user has
#ifdef DECEPTUS_VRSFML
   _window = std::make_shared<sf::RenderWindow>(
      sf::RenderWindow::create(
         sf::WindowSettings{
            .size = {static_cast<uint32_t>(game_config._video_mode_width), static_cast<uint32_t>(game_config._video_mode_height)},
            .title = GAME_NAME,
            .fullscreen = game_config._fullscreen,
            .contextSettings = context_settings
         }
      )
         .value()
   );
#else
   // fullscreen always runs at the desktop resolution, so it is pulled from the desktop rather than
   // remembered anywhere. the windowed size is the only one worth persisting
   _window = std::make_shared<sf::RenderWindow>(
      game_config._fullscreen
         ? sf::VideoMode::getDesktopMode()
         : sf::VideoMode({static_cast<uint32_t>(game_config._windowed_width), static_cast<uint32_t>(game_config._windowed_height)}),
      GAME_NAME,
      game_config._fullscreen ? sf::State::Fullscreen : sf::State::Windowed,
      context_settings
   );

   // the window manager is free to hand out a different size than the one requested, so the size that
   // everything downstream renders against is read back from the window that actually exists
   const auto window_size = _window->getSize();
   game_config._video_mode_width = static_cast<int32_t>(window_size.x);
   game_config._video_mode_height = static_cast<int32_t>(window_size.y);

   // the view is the smallest thing the game can sensibly show, and the menus are laid out for exactly
   // that size, so dragging the border further in than one view is not something to support
   _window->setMinimumSize(sf::Vector2u{static_cast<uint32_t>(game_config._view_width), static_cast<uint32_t>(game_config._view_height)});
#endif

   SplashScreen::show(*_window);

#ifndef DECEPTUS_VRSFML
   _window->setVerticalSyncEnabled(game_config._vsync_enabled);

   // switching vsync off is how a profiling run asks to see the headroom above 60, so the limiter
   // has to step aside with it - otherwise it caps the run at the very number being measured. this
   // matches what the console branch below already does
   _window->setFramerateLimit(game_config._vsync_enabled ? 60 : 0);
#elif defined(__SWITCH__)
   // the console's panel runs at 60 Hz in both modes and paces the loop by itself, so vsync is all
   // that is wanted here and no frame rate limiter goes on top: a limiter would also cap a profiling
   // run, and those deliberately switch vsync off to see how much headroom there is above 60.
   // the browser drives its own loop, which is why the web build gets neither.
   _window->setVerticalSyncEnabled(game_config._vsync_enabled);
#endif
   _window->setKeyRepeatEnabled(false);
   _window->setMouseCursorVisible(!game_config._fullscreen);

   showGpu();

   initializeRenderTargets();
}

void Game::initializeRenderTargets()
{
   const auto& game_config = GameConfiguration::getInstance();

   // reset render textures if needed
   if (_window_render_texture)
   {
      _window_render_texture.reset();
   }

   // dropped rather than resized, the pass rebuilds it at the new size on next use
   _post_processing_pass.release();

   // this the render texture size derived from the window dimensions. as opposed to the window
   // dimensions this one takes the view dimensions into regard and preserves an integer multiplier
   const auto placement = game_config.computeWindowImagePlacement();

   const int32_t texture_width = placement.texture_width;
   const int32_t texture_height = placement.texture_height;

   Log::Info() << "video mode: " << game_config._video_mode_width << " x " << game_config._video_mode_height
               << ", view size: " << game_config._view_width << " x " << game_config._view_height
               << ", ratio: " << game_config.getViewScale();

#ifndef DECEPTUS_VRSFML
   _window_render_texture =
      std::make_shared<sf::RenderTexture>(sf::Vector2u{static_cast<uint32_t>(texture_width), static_cast<uint32_t>(texture_height)});
#else
   _window_render_texture = std::make_shared<sf::RenderTexture>(
      std::move(*sf::RenderTexture::create(sf::Vector2u{static_cast<uint32_t>(texture_width), static_cast<uint32_t>(texture_height)}))
   );
#endif

   Log::Info() << "created window render texture: " << texture_width << " x " << texture_height;

#ifndef DECEPTUS_VRSFML
   // once the blit scale is not a whole number, nearest neighbour hands some view pixels one more screen
   // pixel than their neighbours, and that uneven pattern crawls across the image as the camera moves.
   // smoothing spreads the remainder out instead. a pixel precise blit runs at scale 1 and must not be
   // smoothed at all, or every pixel of the frame gets resampled for nothing
   _window_render_texture->setSmooth(!game_config._preserve_pixel_precision);
#endif

   if (_level)
   {
      _render_targets.recreateOnResize(
         game_config._video_mode_width, game_config._video_mode_height, game_config._view_width, game_config._view_height
      );
   }
   else
   {
      _render_targets.create(
         game_config._video_mode_width, game_config._video_mode_height, game_config._view_width, game_config._view_height
      );
   }
}

void Game::initializeController()
{
   auto& gji = GameControllerIntegration::getInstance();

   gji.addDeviceAddedCallback(
      [this](int32_t /*id*/)
      {
         const auto& gji = GameControllerIntegration::getInstance();
         gji.getController()->addButtonPressedCallback(SDL_GAMEPAD_BUTTON_START, [this]() { showPauseMenu(); });
      }
   );

   gji.initialize();
}

void Game::playMenuMusic()
{
   MusicPlayer::getInstance().queueTrack(
      {.filename = MusicFilenames::getMenuMusic().string(),
       .transition = MusicPlayerTypes::TransitionType::Crossfade,
       .duration = std::chrono::milliseconds(1000),
       .post_action = MusicPlayerTypes::PostPlaybackAction::Loop}
   );
}

void Game::playLevelMusic()
{
   MusicPlayer::getInstance().queueTrack(
      {.filename = MusicFilenames::getLevelMusic().string(),
       .transition = MusicPlayerTypes::TransitionType::Crossfade,
       .duration = std::chrono::milliseconds(1000),
       .post_action = MusicPlayerTypes::PostPlaybackAction::Loop}
   );
}

void Game::showMainMenu()
{
   Menu::getInstance()->show(Menu::MenuType::Main);
   GameState::getInstance().enqueueStop();

   playMenuMusic();
}

void Game::showPauseMenu()
{
   // while the game is loading, don't bother to open the pause screen
   if (!_level_loading_finished)
   {
      return;
   }

   // don't allow to pause during screen transitions
   // don't allow to pause when the inventory is open (game is already paused)
   // don't allow to pause while a cutscene is playing
   if (DisplayMode::getInstance().isAnySet(Display::ScreenTransition, Display::IngameMenu, Display::CutsceneActive))
   {
      return;
   }

   // when there's a dialogue open, opening the pause menu also does not make any sense
   if (DisplayMode::getInstance().isSet(Display::Modal))
   {
      return;
   }

   if (Menu::getInstance()->getCurrentType() == Menu::MenuType::None)
   {
      Menu::getInstance()->show(Menu::MenuType::Pause);
      GameState::getInstance().enqueuePause();
      _audio_callback(GameAudio::SoundEffect::GameStatePause);
      playMenuMusic();
   }
}

void Game::loadLevel(LoadingMode loading_mode)
{
   // Only record the request here; the teardown and the load itself happen at the top of the next
   // frame, in processPendingLevelLoad(). See the note there for why.
   //
   // Flipping the flags right away is what stops update() and draw() touching the outgoing level in
   // the meantime, so callers still get "the level is going away" semantics immediately.
   _level_loading_finished = false;
   _level_loading_finished_previous = false;
   _info_layer->setLoading(true);

   _pending_level_load = loading_mode;
}

void Game::processPendingLevelLoad()
{
   if (!_pending_level_load.has_value())
   {
      return;
   }

   const auto loading_mode = _pending_level_load.value();
   _pending_level_load.reset();

   // Destroy the outgoing level here: on the thread that owns the drawing context, and strictly
   // before the loader thread is started.
   //
   // The thread matters because the loader activates its own sf::Context. Destroying the level there
   // deletes its GL objects - every shader, every texture - from a context other than the one that
   // draws them. Loading then recreates those objects and the driver hands out the same GL handles
   // again, while the drawing context still resolves those handles to the objects it saw before,
   // which have just been freed. The first draw after a reload then walks freed driver memory and
   // dies inside glGetUniformLocation.
   //
   // The ordering matters just as much: a Level constructor resets shared state that the outgoing
   // level's nodes still belong to (LuaInterface::reset() destroys its LuaNodes, and ~GameNode
   // deregisters from its parent). Letting that run on the loader thread while this thread tears the
   // old level down has the two of them mutating the same node lists at once. Destroying first, then
   // loading, keeps it sequential.
   //
   // And doing all of it a frame late rather than inside loadLevel() is what keeps it off the level's
   // own call stack: a load can be requested from inside the level's update, by a lua script calling
   // nextLevel().
   _player->resetWorld();  // free the pointer that's shared with the player
   LevelRegistry::clearCurrent();

   // the intermediate post processing target belongs to the outgoing level's effects, so it is
   // released here rather than kept for a level that may never ask for it. it is deliberately not
   // released when an effect merely switches off: a trigger area toggling as the player walks in
   // and out would otherwise reallocate a full screen target on every crossing
   _post_processing_pass.release();

   const auto teardown_start = std::chrono::steady_clock::now();
   _level.reset();
   const auto teardown_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - teardown_start).count();

   // only worth mentioning when it actually cost a frame
   if (teardown_ms > 16)
   {
      Log::Info() << "previous level torn down in " << teardown_ms << "ms";
   }

   const auto level_loader = [this, loading_mode]()
   {
   // create an opengl context for this thread
#ifndef DECEPTUS_VRSFML
      sf::Context loader_context;
      loader_context.setActive(true);
#endif

      // load level
      const auto level_item = Levels::readLevelItem(SaveState::getCurrent()._level_index);
      _level = std::make_shared<Level>(_render_targets);
      LevelRegistry::setCurrent(_level);
      _level->setDescriptionFilename(level_item._level_name);
      _level->setLoadingMode(loading_mode);
      _level->initialize();

      // put the player in there
      _player->setWorld(_level->getWorld());
      _player->initializeLevel();

      // re-equip items now that level and player are both live; items deserialized before the
      // level was ready (e.g. head torch) had their onEquipped() silently no-op at that point
      SaveState::getPlayerInfo()._items.reinitializeEquippedItems();

      // jump back to stored position, that's only for debugging purposes, not for checkpoints
      if (_restore_previous_position)
      {
         _restore_previous_position = false;
         _player->setBodyViaPixelPosition(_stored_position.x, _stored_position.y);
      }

      _player->updatePixelRect();

      Log::Info() << "level loading finished: " << level_item._level_name;

      _level_loading_finished = true;

      // loading took seconds of real time that the simulation must not make up for, or the first
      // frame back would run its whole catch-up allowance at once
      _fixed_time_step.reset();

      playLevelMusic();

      // before synchronizing the camera with the player position, the camera needs to know its room limitations
      _level->syncRoom();

      CameraSystem::getInstance().syncNow();
      GameClock::getInstance().reset();

      _info_layer->setLoading(false);

      // notify listeners
      for (const auto& callback : _level_loaded_callbacks)
      {
         callback();
      }

      _level_loaded_callbacks.clear();

#ifndef DECEPTUS_VRSFML
      loader_context.setActive(false);
#endif
   };

   // Loading synchronously on the VRSFML targets is not a threading limitation, whatever the
   // guard's name suggests -- the Switch has real pthreads through libnx and runs the log
   // thread happily. It is a GL context limitation, and it is hard on both targets: the loader
   // above brings up an sf::Context of its own and relies on it sharing objects with the render
   // context. WebGL has no context sharing at all, and devkitPro's switch-mesa accepts a share
   // list and then silently ignores it, so anything the loader uploaded would be invisible to
   // the renderer. See switch_port_status.md.
#ifdef DECEPTUS_VRSFML
   level_loader();
#else
   _level_loading_thread = std::async(std::launch::async, level_loader);
#endif
}

void Game::nextLevel()
{
   SaveState::getCurrent()._level_index++;

   auto levels = Levels::readLevelItems();
   if (SaveState::getCurrent()._level_index == static_cast<int32_t>(levels.size()))
   {
      // this could show the end sequence or similar
      // DrawStates::_draw_test_scene = true;

      SaveState::getCurrent()._level_index = 0;
   }

   loadLevel();
}

Game::~Game()
{
   EventSerializer::unregisterInstance("global");
   LevelRegistry::clearCurrent();
   _level.reset();
}

void Game::initialize()
{
   auto& config = GameConfiguration::getInstance();

   // clamp resolution to desktop limits on startup
   config.clampResolutionToDesktop();

   initializeWindow();

   // initialize GLEW after the OpenGL context is created but before any OpenGL calls
   if (!_window_render_texture->setActive(true))
   {
      Log::Error() << "Failed to activate render texture";
      return;
   }

   const auto glew_error = glewInit();
   if (glew_error != GLEW_OK)
   {
      Log::Error() << "Failed to initialize GLEW: " << glew_error;
      return;
   }

   initializeController();

   PostProcessing::getInstance().initialize();

   _player = std::make_shared<Player>();
   PlayerRegistry::add(_player);
   _player->initialize();

   _global_event_serializer = std::make_shared<EventSerializer>();
   _global_event_serializer->setCallback([this](const sf::Event& event) { processEvent(event); });
   EventSerializer::registerInstance("global", _global_event_serializer);

   _info_layer = std::make_unique<InfoLayer>();
   _ingame_menu = std::make_unique<InGameMenu>();
   _controller_overlay = std::make_unique<ControllerOverlay>();
#ifndef DECEPTUS_VRSFML
   _test_scene = std::make_unique<ForestScene>();
#endif
   _menu_background = std::make_unique<MenuBackgroundScene>();

   CallbackMap::getInstance().addCallback(static_cast<int32_t>(CallbackType::NextLevel), [this]() { nextLevel(); });
   CallbackMap::getInstance().addCallback(static_cast<int32_t>(CallbackType::LoadLevel), [this]() { loadLevel(); });

   Audio::getInstance();

   // initially the game should be in main menu and paused
   std::dynamic_pointer_cast<MenuScreenMain>(Menu::getInstance()->getMenuScreen(Menu::MenuType::Main))
      ->setExitCallback(
         [this]()
         {
#ifndef DECEPTUS_VRSFML
            _window->close();
#endif
         }
      );

   std::dynamic_pointer_cast<MenuScreenVideo>(Menu::getInstance()->getMenuScreen(Menu::MenuType::Video))
      ->setFullscreenCallback([this]() { toggleFullScreen(); });

   std::dynamic_pointer_cast<MenuScreenVideo>(Menu::getInstance()->getMenuScreen(Menu::MenuType::Video))
      ->setResolutionCallback([this](int32_t w, int32_t h) { changeResolution(w, h); });

   std::dynamic_pointer_cast<MenuScreenVideo>(Menu::getInstance()->getMenuScreen(Menu::MenuType::Video))
      ->setScalingCallback([this]() { applyScalingOptions(); });

   std::dynamic_pointer_cast<MenuScreenVideo>(Menu::getInstance()->getMenuScreen(Menu::MenuType::Video))
      ->setVSyncCallback(
         [this]()
         {
            initializeWindow();
#ifndef DECEPTUS_VRSFML
            _menu_background = std::make_unique<MenuBackgroundScene>();
#endif
            if (!_level)
            {
               return;
            }
            _level->createViews();
         }
      );

   showMainMenu();

   Timer::add(std::chrono::milliseconds(1000), [this]() { updateWindowTitle(); }, Timer::Type::Repeated, Timer::Scope::UpdateAlways);

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

   GameAudio::getInstance().initialize();
   _audio_callback = [](GameAudio::SoundEffect effect) { GameAudio::getInstance().play(effect); };

#ifdef DEVELOPMENT_MODE
   writeMechanismSchemas();
#endif

#if defined(DEVELOPMENT_MODE) && defined(__SWITCH__)
   // the console has no F10 to toggle profiling with and no window to show it in, so it is switched
   // on from the start and reports into the sd card log, which is the only artefact a run on real
   // hardware leaves behind
   _profiling_ui = std::make_unique<ProfilingUi>();
#endif
}

// frambuffers
// - the window render texture
//    - the level render texture
//       - the level background render texture
//    - info layer
//    - menus
//    - inventory
//    - message boxes

void Game::draw()
{
   _fps++;

#ifdef DEVELOPMENT_MODE
   _draw_section_timer.begin((_profiling_ui != nullptr) && _profiling_ui->isMechanismProfilingWanted());
#endif

   _window->clear(sf::Color::Black);
#ifndef DECEPTUS_VRSFML
   _window->pushGLStates();
#endif

   _window_render_texture->clear();

   // a level-scoped effect routes the level through an intermediate target so it can be resolved
   // into the window render texture before any overlay is drawn on top of it
   PostProcessing::getInstance().setLevelEffect(_level_loading_finished && _level ? _level->getActivePostProcessingMechanism() : nullptr);

   const auto level_target = _post_processing_pass.selectLevelTarget(_window_render_texture, _level_loading_finished);

#ifdef DEVELOPMENT_MODE
   _draw_section_timer.mark("game clear targets");
#endif

   if (_level_loading_finished)
   {
      _level->draw(level_target, _screenshot);
   }

#ifdef DEVELOPMENT_MODE
   // the level's own passes are reported separately, so this mark only closes the gap around them
   _draw_section_timer.mark("level draw");
#endif

   _post_processing_pass.resolveLevelTarget(*_window_render_texture.get(), _render_targets.view_to_texture_scale);

   _screenshot = false;

   ScreenTransitionHandler::getInstance().draw(_window_render_texture);

#ifdef DEVELOPMENT_MODE
   _draw_section_timer.mark("post processing resolve");
#endif

   _info_layer->draw(*_window_render_texture.get());

#ifdef DEVELOPMENT_MODE
   _draw_section_timer.mark("hud");
#endif

   if (DebugDrawStates::_draw_debug_info)
   {
      _info_layer->drawDebugInfo(*_window_render_texture.get());
   }

   if (DebugDrawStates::_draw_console)
   {
      _info_layer->drawConsole(*_window_render_texture.get());
   }

   if (DebugDrawStates::_draw_camera_system)
   {
      DebugDraw::debugCameraSystem(*_window_render_texture.get());
   }

   if (DebugDrawStates::_draw_controller_overlay)
   {
      _controller_overlay->draw(*_window_render_texture.get());
   }

   if (DisplayMode::getInstance().isSet(Display::IngameMenu))
   {
      _ingame_menu->draw(*_window_render_texture.get());
   }

#ifndef DECEPTUS_VRSFML
   if (DebugDrawStates::_draw_test_scene)
   {
      _test_scene->draw(*_window_render_texture.get());
   }
#endif

   if (GameState::getInstance().getMode() == ExecutionMode::NotRunning)
   {
#ifdef DECEPTUS_VRSFML
      // raw OpenGL interop under VRSFML: activate the render texture's framebuffer so the 3D
      // scene draws into it. VRSFML auto-batches draws, so first flush any pending SFML geometry
      // while its GL-state cache is still valid; issue the raw OpenGL calls; then re-sync the
      // cache so the subsequent SFML draws rebind their program/buffers correctly.
      if (_window_render_texture->setActive(true))
      {
         _window_render_texture->resetGLStates();
         _menu_background->render(*_window_render_texture);
         _window_render_texture->resetGLStates();
      }
#else
      _menu_background->render(*_window_render_texture);
#endif
   }

#ifdef DECEPTUS_VRSFML
   Menu::getInstance()->draw(*_window_render_texture.get(), sf::RenderStates{.blendMode = sf::BlendAlpha});
#else
   Menu::getInstance()->draw(*_window_render_texture.get(), {sf::BlendAlpha});
#endif
   MessageBox::draw(*_window_render_texture.get());

#ifdef DEVELOPMENT_MODE
   _draw_section_timer.mark("menu and overlays");
#endif

   _window_render_texture->display();

#ifdef DEVELOPMENT_MODE
   _draw_section_timer.mark("window texture display");
#endif
#ifdef DECEPTUS_VRSFML
   _window->setActive(true);
#endif
#ifdef DECEPTUS_VRSFML
   const sf::Texture& window_render_texture_ref = _window_render_texture->getTexture();
   sf::Sprite window_texture_sprite;
   window_texture_sprite.textureRect = sf::FloatRect{
      {0.f, 0.f}, {static_cast<float>(window_render_texture_ref.getSize().x), static_cast<float>(window_render_texture_ref.getSize().y)}
   };
#else
   auto window_texture_sprite = sf::Sprite(_window_render_texture->getTexture());
#endif

   // fullscreen and windowed pose the same question - where does a whole multiple of the view sit inside a
   // window that is not itself a multiple of it - so both go through the same placement now. the branch
   // that used to be here derived the leftover space from the window size rather than the video mode,
   // which are kept equal, and so arrived at the same numbers by a longer route
   const auto placement = GameConfiguration::getInstance().computeWindowImagePlacement();
   sfcompat::setPosition(window_texture_sprite, {placement.offset_x, placement.offset_y});
#ifdef DECEPTUS_VRSFML
   window_texture_sprite.scale = {placement.scale_x, placement.scale_y};
#else
   window_texture_sprite.scale({placement.scale_x, placement.scale_y});
#endif

   // a frame-scoped effect runs here, on the fully composited frame, i.e. on top of the level's
   // gamma pass as well as on all overlays. a level-scoped one has already been resolved above.
   const auto* post_processing_shader = _post_processing_pass.getFrameShader(_window_render_texture->getTexture());

#ifdef DECEPTUS_VRSFML
   _window->draw(window_texture_sprite, sf::RenderStates{.texture = &window_render_texture_ref, .shader = post_processing_shader});
#else
   _window->draw(window_texture_sprite, post_processing_shader);
#endif
#ifndef DECEPTUS_VRSFML
   _window->popGLStates();
#endif

#ifdef DEVELOPMENT_MODE
   _draw_section_timer.mark("window blit");
#endif

#ifdef DEVELOPMENT_MODE
   sf::Clock window_display_clock;
#endif
   _window->display();
#ifdef DEVELOPMENT_MODE
   if (_profiling_ui)
   {
      _profiling_ui->recordWindowDisplay(window_display_clock.getElapsedTime());
   }
#endif

#ifndef DECEPTUS_VRSFML
   if (_recording)
   {
      const auto image = window_texture_sprite.getTexture().copyToImage();

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
#endif

   if (DebugDrawStates::_draw_physics_config)
   {
      _physics_ui->draw();
   }

   if (DebugDrawStates::_draw_camera_system)
   {
      _camera_ui->draw();
   }

#ifndef DECEPTUS_VRSFML
   if (DebugDrawStates::_draw_log)
   {
      _log_ui->draw();
   }
#endif

#ifdef DEVELOPMENT_MODE
   if (_profiling_ui)
   {
      _profiling_ui->draw();
   }
#endif
}

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

void Game::updateWindowTitle()
{
   std::ostringstream out_stream;
   out_stream << GAME_NAME << " - " << _fps << "fps";
#ifdef DEVELOPMENT_MODE
   out_stream << " [" << DECEPTUS_BUILD_TYPE << "]";
#endif
#ifdef DECEPTUS_VRSFML
   _window->setTitle(out_stream.str().c_str());
#else
   _window->setTitle(out_stream.str());
#endif
   _fps = 0;
}

void Game::goToLastCheckpoint()
{
   SaveState::deserializeFromFile();
   _player->reset();
   loadLevel();
}

void Game::menuLoadRequest()
{
   // the code below is mostly identical to 'goToLastCheckpoint'
   // however, this does not deserialize the last game state; anyhow - duplication should be removed
   ScreenTransitionHandler::getInstance().clear();
   _player->reset();
   loadLevel();

   // fade in after level loading is done
   _level_loaded_callbacks.push_back(
      []
      {
         auto screen_transition = makeFadeInAfterLoadGame();
         screen_transition->_callbacks_effect_2_ended.emplace_back([]() { ScreenTransitionHandler::getInstance().pop(); });
         ScreenTransitionHandler::getInstance().push(std::move(screen_transition));
         ScreenTransitionHandler::getInstance().startEffect2();
      }
   );
}

void Game::resetAfterDeath(const sf::Time& dt)
{
   // not great. the screen transitions drive the level loading and game workflow.
   // it should rather be the other way round. on the other hand this approach allows very simple
   // timing and the fading is very unlikely to fail anyway.
   if (_player->isDead())
   {
      _death_wait_time_ms += dt.asMilliseconds();

      if (_death_wait_time_ms > 1000 && !ScreenTransitionHandler::getInstance().active())
      {
         // fade out/in
         // do the actual level reset once the fade out has happened
         auto screen_transition = makeFadeOutFadeInDeath();
         screen_transition->_callbacks_effect_1_ended.emplace_back([this]() { goToLastCheckpoint(); });
         screen_transition->_callbacks_effect_2_ended.emplace_back([]() { ScreenTransitionHandler::getInstance().pop(); });
         ScreenTransitionHandler::getInstance().push(std::move(screen_transition));
      }
   }

   if (_level_loading_finished && !_level_loading_finished_previous)
   {
      _level_loading_finished_previous = true;
      ScreenTransitionHandler::getInstance().startEffect2();
   }
}

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

void Game::update()
{
   const auto dt = _delta_clock.getElapsedTime();
   _delta_clock.restart();

   Timer::update(Timer::Scope::UpdateAlways);
   MusicPlayer::getInstance().update(dt);
   MessageBox::update(dt);
   PostProcessing::getInstance().update(dt);

   // update screen transitions here
   ScreenTransitionHandler::getInstance().update(dt);

   // reload the level when the save state has been invalidated, that means when a state is selected from the menu
   if (SaveState::getCurrent()._load_level_requested)
   {
      SaveState::getCurrent()._load_level_requested = false;
      menuLoadRequest();
   }

   const auto game_mode = GameState::getInstance().getMode();

   Menu::getInstance()->update(dt);

   if (game_mode == ExecutionMode::NotRunning)
   {
      updateGameController();
      _menu_background->update(dt);
   }

   _info_layer->update(dt);

   if (game_mode == ExecutionMode::Paused)
   {
      updateGameController();

      if (DisplayMode::getInstance().isSet(Display::IngameMenu))
      {
         _ingame_menu->update(dt);
      }
   }
   else if (game_mode == ExecutionMode::Running)
   {
      Timer::update(Timer::Scope::UpdateIngame);

      if (_level_loading_finished)
      {
         updateGameController();
         updateGameControllerForGame();

         // the simulation is stepped at a fixed rate rather than once per frame. everything below
         // this line was written expecting to run exactly once per physics step, with a delta of
         // PhysicsConfiguration::_time_step - the player's jump and dash forces, the conveyor belt
         // state, the contact events. Feeding it the frame time instead made the world run at double
         // speed at 120 fps and in slow motion below 60, so the fix is to restore that expectation
         // in this one place rather than to teach every one of those about the frame rate.
         //
         // A frame faster than one step runs the loop zero times and draws the same state again.
         const auto simulation_step_count = _fixed_time_step.consumeSteps(dt);
         const auto simulation_dt = _fixed_time_step.getStepDuration();

         for (auto simulation_step = 0; simulation_step < simulation_step_count; simulation_step++)
         {
            EventSerializer::updateAll(simulation_dt);

            _level->update(simulation_dt);
            _player->update(simulation_dt);

            // a lua script can request a level change from inside the update above, which hands
            // _level over for teardown. Stepping it again would run a level that is already gone or
            // already asking to be replaced
            if (!_level || _level->isDirty())
            {
               break;
            }
         }

         // once per frame, after the steps: the simulation has moved, so the sprites are placed
         // where they sit between the last two steps. Positions belong in update, draw draws
         if (_level)
         {
            _level->updateSpritePositions();
            _player->updateSpritePositions();
         }

#ifndef DECEPTUS_VRSFML
         if (DebugDrawStates::_draw_test_scene)
         {
            _test_scene->update(dt);
         }
#endif

         // mechanisms file their transition requests while the level updates, so pick them up right after
         LevelTransitionHandler::getInstance().update();

         // this might trigger level-reloading, so this ought to be the last drawing call in the loop
         updateGameState(dt);

         // a lua script can request a level change from inside the update above, which hands _level
         // over for teardown, so it is not necessarily still there by the time we get here
         if (_level && _level->isDirty())
         {
            reloadLevel(LoadingMode::Clean);
         }
      }
   }

   GameState::getInstance().sync();
   DisplayMode::getInstance().sync();
}

void Game::timedUpdate()
{
#ifdef DEVELOPMENT_MODE
   sf::Clock update_clock;
#endif
   update();
#ifdef DEVELOPMENT_MODE
   _profiling_update_elapsed = update_clock.getElapsedTime();
#endif
}

void Game::timedDraw()
{
#ifdef DEVELOPMENT_MODE
   sf::Clock draw_clock;
#endif
   draw();
#ifdef DEVELOPMENT_MODE
   const auto draw_elapsed = draw_clock.getElapsedTime();
   if (_profiling_ui)
   {
      _profiling_ui->recordFrame(_profiling_update_elapsed + draw_elapsed, _profiling_update_elapsed, draw_elapsed);
   }
   if (_level)
   {
      const auto mechanism_profiling_enabled = (_profiling_ui != nullptr) && _profiling_ui->isMechanismProfilingWanted();
      _level->setMechanismProfilingEnabled(mechanism_profiling_enabled);
      // no level means no passes worth reporting, and the sections would otherwise be written every
      // few seconds from the menu for nothing
      if (mechanism_profiling_enabled && _level_loading_finished)
      {
         _profiling_ui->updateMechanismTimings(_level->getMechanismTimings(32));

         // Level::draw's passes plus everything else Game::draw does, so the report can hold the
         // total against the measured draw time and show what is left unaccounted for
         auto section_timings = _level->getRenderSectionTimings();
         const auto& draw_sections = _draw_section_timer.samples();
         section_timings.insert(section_timings.end(), draw_sections.begin(), draw_sections.end());
         _profiling_ui->updateRenderSectionTimings(std::move(section_timings));
      }
   }
#endif
}

int32_t Game::loop()
{
// the browser drives its own main loop, so this branch is genuinely emscripten-specific
// rather than a question of which SFML flavour is in use; the switch runs the ordinary
// while loop below
#ifdef __EMSCRIPTEN__
   // re-fit the render resolution whenever the browser/itch viewport changes size (window resize,
   // fullscreen toggle) so the game keeps filling the window at an integer scale
   emscripten_set_resize_callback(
      EMSCRIPTEN_EVENT_TARGET_WINDOW,
      this,
      EM_FALSE,
      [](int, const EmscriptenUiEvent*, void* user_data) -> EM_BOOL
      {
         static_cast<Game*>(user_data)->refitToViewport();
         return EM_FALSE;
      }
   );

   emscripten_set_main_loop_arg(
      [](void* arg)
      {
         Game* game = static_cast<Game*>(arg);
         game->processPendingLevelLoad();
         game->processEvents();
         game->timedUpdate();
         game->timedDraw();
      },
      this,
      0,
      1
   );
   return 0;
#elif defined(__SWITCH__)
   // VRSFML dropped sf::RenderWindow::isOpen(), and on the switch the authority on
   // whether the game should keep running is libnx anyway: appletMainLoop() goes false
   // when the system wants the applet gone, e.g. the user quitting from the home menu.
   while (appletMainLoop())
   {
      processPendingLevelLoad();
      processEvents();
      timedUpdate();
      timedDraw();
   }

   return 0;
#else
   while (_window->isOpen())
   {
      processPendingLevelLoad();
      processEvents();
      timedUpdate();
      timedDraw();
   }

   return 0;
#endif
}

void Game::reset()
{
   _player->reset();
}

void Game::toggleFullScreen()
{
#ifndef DECEPTUS_VRSFML
   auto& config = GameConfiguration::getInstance();
   config._fullscreen = !config._fullscreen;

   // since stencil buffers are used, it is required to enable them explicitly
   sf::ContextSettings context_settings;
   context_settings.stencilBits = 8;

   if (config._fullscreen)
   {
      // save current windowed dimensions before switching
      config._windowed_width = config._video_mode_width;
      config._windowed_height = config._video_mode_height;

      auto desktop_mode = sf::VideoMode::getDesktopMode();
      _window->create(desktop_mode, GAME_NAME, sf::Style::None, sf::State::Fullscreen, context_settings);

      // update active resolution to match desktop
      config._video_mode_width = desktop_mode.size.x;
      config._video_mode_height = desktop_mode.size.y;
   }
   else
   {
      // restore windowed mode with saved dimensions
      config._video_mode_width = config._windowed_width;
      config._video_mode_height = config._windowed_height;

      _window->create(
         sf::VideoMode({static_cast<uint32_t>(config._video_mode_width), static_cast<uint32_t>(config._video_mode_height)}),
         GAME_NAME,
         sf::Style::Default,
         sf::State::Windowed,
         context_settings
      );
   }

   // a freshly created window does not carry the constraint the previous one was given
   _window->setMinimumSize(sf::Vector2u{static_cast<uint32_t>(config._view_width), static_cast<uint32_t>(config._view_height)});

   config.serializeToFile();

#ifndef DECEPTUS_VRSFML
   _window->setVerticalSyncEnabled(config._vsync_enabled);
   _window->setFramerateLimit(config._vsync_enabled ? 60 : 0);
#elif defined(__SWITCH__)
   _window->setVerticalSyncEnabled(config._vsync_enabled);
#endif
   _window->setKeyRepeatEnabled(false);
   _window->setMouseCursorVisible(!config._fullscreen);

   // the new window needs a render texture and level targets sized for it, which is what this does. it
   // also picks the texture filtering that goes with the current scaling options, which a hand rolled
   // copy of it here would have to remember to keep in step
   initializeRenderTargets();

   if (_level)
   {
      _level->createViews();
   }

   // recreate the 3D menu background — its GL resources are tied to the old context
   _menu_background = std::make_unique<MenuBackgroundScene>();
#endif
}

void Game::applyScalingOptions()
{
#ifndef DECEPTUS_VRSFML
   // the scaling options move nothing but the blit, and the blit reads them fresh out of the config every
   // frame, so there is nothing to rebuild here. the one piece of state that has to follow them is the
   // filtering: a fractional blit has to interpolate and a pixel precise one must not.
   //
   // this used to call initializeRenderTargets, which tore down and rebuilt every render texture on each
   // keypress even though not one of them depends on these options - their sizes come from the whole
   // number view scale, which the options do not touch. doing that under the menu, whose background holds
   // raw gl state of its own, crashed the game after a handful of toggles
   auto& config = GameConfiguration::getInstance();

   // switching pixel precision on tightens the window down onto the image, so there is nothing left over
   // to put bars in. it is done here and not from the resize handler on purpose: re-snapping on every
   // border drag would make the window impossible to size by hand, and dragging it afterwards simply
   // letterboxes again until the option is switched on afresh.
   //
   // fullscreen has no window of its own to tighten - it runs at whatever the desktop hands out - so it
   // keeps letterboxing. the snapped size is a whole multiple of the view by construction, so the whole
   // number scale cannot change and no render target is ever rebuilt by this
   if (config._preserve_pixel_precision && !config._fullscreen)
   {
      const auto placement = config.computeWindowImagePlacement();
      _window->setSize({static_cast<uint32_t>(placement.texture_width), static_cast<uint32_t>(placement.texture_height)});
      adoptWindowSize(placement.texture_width, placement.texture_height);
   }

   _window_render_texture->setSmooth(!config._preserve_pixel_precision);
#endif
}

void Game::adoptWindowSize(int32_t width, int32_t height)
{
   auto& config = GameConfiguration::getInstance();

   if (width == config._video_mode_width && height == config._video_mode_height)
   {
      return;
   }

   const auto previous_view_scale = config.getViewScale();

   config._video_mode_width = width;
   config._video_mode_height = height;

#ifndef DECEPTUS_VRSFML
   // sfml leaves a window's view alone across a resize - RenderWindow::onResize only recomputes the
   // viewport, which is stored in relative coordinates, and the view keeps the world size it was built
   // with. so it goes on mapping the old size onto the new window, which draws the composited frame too
   // small when the border is dragged inwards and too large when it is dragged outwards. recreating the
   // window used to hide this behind a brand new default view
   _window->setView(sf::View{sf::FloatRect{{0.0f, 0.0f}, {static_cast<float>(width), static_cast<float>(height)}}});
#endif

   // fullscreen runs at whatever the desktop hands out, so only a windowed size is worth remembering.
   // this is not written to disk here: dragging a window border fires an event per step, and the file
   // would be rewritten for every one of them. it goes out when the game closes instead
   if (!config._fullscreen)
   {
      config._windowed_width = width;
      config._windowed_height = height;
   }

   // every render target is sized as a whole multiple of the view rather than to the window, so dragging
   // a border around within one multiple leaves all of them at the right size and only moves the blit.
   // rebuilding the whole set per resize event would make dragging stutter for nothing
   if (config.getViewScale() == previous_view_scale)
   {
      return;
   }

   // the window already has this size, and it already sits on the monitor it was dragged to. going
   // through changeResolution would recreate it, and a freshly created window gets default placement -
   // the primary monitor - which is what threw the window back onto the first screen when it was dragged
   // onto a second one. nothing here needs a new window, only render targets that match the new size
   initializeRenderTargets();

   if (_level)
   {
      _level->createViews();
   }
}

void Game::changeResolution(int32_t w, int32_t h)
{
   auto& config = GameConfiguration::getInstance();

   config._video_mode_width = w;
   config._video_mode_height = h;

   // if in windowed mode, save these dimensions for later restoration
   if (!config._fullscreen)
   {
      config._windowed_width = w;
      config._windowed_height = h;
      config.serializeToFile();
   }

   // clamp to desktop limits to prevent SFML errors
   config.clampResolutionToDesktop();

#ifdef DECEPTUS_VRSFML
   // recreating the render window would drop the browser out of fullscreen, since the new window is
   // created with fullscreen=false. entering fullscreen resizes the viewport, which invokes this via
   // refitToViewport(), so a recreation here makes the game kick itself straight back to windowed.
   // resizing the existing canvas keeps the fullscreen state and the gl context intact.
   _window->setSize({static_cast<uint32_t>(config._video_mode_width), static_cast<uint32_t>(config._video_mode_height)});
   initializeRenderTargets();
#else
   initializeWindow();

   _menu_background = std::make_unique<MenuBackgroundScene>();
#endif

   if (_level)
   {
      _level->createViews();
   }
}

#ifdef DECEPTUS_VRSFML
void Game::refitToViewport()
{
   auto& config = GameConfiguration::getInstance();
   const auto [new_width, new_height] = config.computeViewportVideoMode();
   if (new_width == config._video_mode_width && new_height == config._video_mode_height)
   {
      return;
   }

   changeResolution(new_width, new_height);
}
#endif

void Game::takeScreenshot()
{
   _screenshot = true;
}

void Game::processEvent(const sf::Event& event)
{
   _global_event_serializer->add(event);

   const auto invokePlayerControls = [this](auto&& func)
   {
      if (_player && _player->getControls())
      {
         func();
      }
   };

   invokePlayerControls([&event, this]() { _player->getControls()->handleEvent(event); });

   if (event.is<sf::Event::Closed>())
   {
      // resizes deliberately do not write the file, so this is where a dragged window size is kept
      GameConfiguration::getInstance().serializeToFile();

#ifndef DECEPTUS_VRSFML
      _window->close();
#endif
   }
   else if (auto* resized_event = event.getIf<sf::Event::Resized>())
   {
#ifdef __linux__
      return;
#endif
#ifdef DECEPTUS_VRSFML
      // on the web the canvas is resized by the game itself (refitToViewport), so acting on the
      // resulting event would feed arbitrary, non-integer-multiple sizes back into changeResolution
      return;
#endif
      adoptWindowSize(static_cast<int32_t>(resized_event->size.x), static_cast<int32_t>(resized_event->size.y));
   }
   else if (event.is<sf::Event::FocusLost>())
   {
      if (GameConfiguration::getInstance()._pause_mode == GameConfiguration::PauseMode::AutomaticPause)
      {
         // the in-game menu is save to leave open when losing the window focus
         // while the console is open, don't disturb
         if (!DisplayMode::getInstance().isSet(Display::IngameMenu) && !DebugDrawStates::_draw_console)
         {
            showPauseMenu();
         }
      }
      else
      {
         invokePlayerControls(
            [this]()
            {
               CameraPanorama::getInstance().updateLookState(Look::Active, false);
               DisplayMode::getInstance().enqueueUnset(Display::CameraPanorama);
               _player->getControls()->setKeysPressed(0);
            }
         );
      }
   }
   else if (event.is<sf::Event::FocusGained>())
   {
      invokePlayerControls([this]() { _player->getControls()->forceSync(); });
   }
   else if (auto* key_pressed_event = event.getIf<sf::Event::KeyPressed>())
   {
      if (key_pressed_event->code == sf::Keyboard::Key::F11)
      {
         toggleFullScreen();
         return;
      }

      if (MessageBox::keyboardKeyPressed(key_pressed_event->code))
      {
         // nom nom nom
         return;
      }

      if (DebugDrawStates::_draw_console)
      {
#ifdef DEVELOPMENT_MODE
         Console::getInstance().processEvent(key_pressed_event->code);
         return;
#endif
      }
      else
      {
         if (Menu::getInstance()->isVisible())
         {
            Menu::getInstance()->keyboardKeyPressed(key_pressed_event->code);
            return;
         }

         invokePlayerControls([key_pressed_event, this]() { _player->getControls()->keyboardKeyPressed(key_pressed_event->code); });
      }

      // this is the handling of the actual in-game keypress events
      processKeyPressedEvents(key_pressed_event);
   }
   else if (auto* key_released_event = event.getIf<sf::Event::KeyReleased>())
   {
      if (Menu::getInstance()->isVisible())
      {
         Menu::getInstance()->keyboardKeyReleased(key_released_event->code);
         return;
      }

      invokePlayerControls([key_released_event, this]() { _player->getControls()->keyboardKeyReleased(key_released_event->code); });
      processKeyReleasedEvents(key_released_event);
   }

#ifdef DEVELOPMENT_MODE
   else if (auto* text_entered_event = event.getIf<sf::Event::TextEntered>())
   {
      if (DebugDrawStates::_draw_console)
      {
         Console::getInstance().append(text_entered_event->unicode);
         return;
      }
   }
   else if (auto* mouse_button_pressed_event = event.getIf<sf::Event::MouseButtonPressed>())
   {
      if (mouse_button_pressed_event->button == sf::Mouse::Button::Right)
      {
         if (LevelRegistry::getCurrent())
         {
#ifndef DECEPTUS_VRSFML
            const auto mouse_pos_px = sf::Mouse::getPosition(*_window);
            const auto game_coords_px = _window->mapPixelToCoords(mouse_pos_px, *LevelRegistry::getCurrent()->getLevelView());
            PlayerRegistry::getFirst()->setBodyViaPixelPosition(game_coords_px.x, game_coords_px.y);
#endif
         }
      }
   }
   else if (auto* mouse_wheel_scrolled_event = event.getIf<sf::Event::MouseWheelScrolled>())
   {
      LevelRegistry::getCurrent()->zoomBy(mouse_wheel_scrolled_event->delta);
   }
#endif

   EventDistributor::event(event);
}

void Game::shutdown()
{
   // quitting from the menu never goes past a Closed event, so the window size is written out here too
   GameConfiguration::getInstance().serializeToFile();

   if (_physics_ui)
   {
      _physics_ui->close();
   }

   if (_camera_ui)
   {
      _camera_ui->close();
   }

#ifndef DECEPTUS_VRSFML
   if (_log_ui)
   {
      _log_ui->close();
   }
#endif

   // std::exit skips local destructors in main(), so Level and all its children (LevelScript,
   // Lever, etc.) would otherwise be torn down during static cleanup — after SaveState::__save_states
   // may already be gone. Destroy the level explicitly here while all statics are still alive.
   LevelRegistry::clearCurrent();
   _level.reset();

   std::exit(0);
}

void Game::reloadLevel(LoadingMode loading_mode)
{
   if (!_level_loading_finished)
   {
      return;
   }

   // so when a dialogue is open and dstar modifies the tmx file and saves, then the game is reloaded.
   // that'll leave the dialogue open which will call the callback which is gonna be invalid after re-loading.
   // just resetting the messagebox before will just delete the messagebox.
   if (DisplayMode::getInstance().isSet(Display::Modal))
   {
      MessageBox::reset();
   }

   _restore_previous_position = true;
   _stored_position = _player->getPixelPositionFloat();
   _player->reset();
   loadLevel(loading_mode);
}

void Game::processKeyPressedEvents(const sf::Event::KeyPressed* key_event)
{
   if (DisplayMode::getInstance().isSet(Display::IngameMenu))
   {
      _ingame_menu->processEvent(key_event);
      return;
   }

   CameraPanorama::getInstance().processKeyPressedEvents(key_event);

   if (_player && _player->getControls() && _player->getControls()->hasFlag(KeyPressedInventory))
   {
      _ingame_menu->open();
      return;
   }

   switch (key_event->code)
   {
      case sf::Keyboard::Key::F:
      {
         toggleFullScreen();
         break;
      }
      case sf::Keyboard::Key::P:
      case sf::Keyboard::Key::Escape:
      {
         showPauseMenu();
         break;
      }

#ifdef DEVELOPMENT_MODE
      case sf::Keyboard::Key::F10:
      {
         if (!_profiling_ui)
         {
            _profiling_ui = std::make_unique<ProfilingUi>();
         }
         else
         {
            _profiling_ui->close();
            _profiling_ui.reset();
         }
         break;
      }
#endif

#ifdef DEVELOPMENT_MODE
      case sf::Keyboard::Key::G:
      {
         const auto scale = PlayerRegistry::getFirst()->getBody()->GetGravityScale();
         PlayerRegistry::getFirst()->getBody()->SetGravityScale(scale < 0.0f ? 1.0f : -0.1f);
         break;
      }
      case sf::Keyboard::Key::F1:
      {
         DisplayMode::getInstance().enqueueToggle(Display::Debug);
         break;
      }
      case sf::Keyboard::Key::F2:
      {
         DebugDrawStates::_draw_controller_overlay = !DebugDrawStates::_draw_controller_overlay;
         break;
      }
      case sf::Keyboard::Key::F3:
      {
         DebugDrawStates::_draw_camera_system = !DebugDrawStates::_draw_camera_system;
         if (DebugDrawStates::_draw_camera_system && !_camera_ui)
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
      case sf::Keyboard::Key::F4:
      {
         if (key_event->alt)
         {
            shutdown();
         }
         DebugDrawStates::_draw_debug_info = !DebugDrawStates::_draw_debug_info;
         break;
      }
      case sf::Keyboard::Key::F5:
      {
#ifndef DECEPTUS_VRSFML
         DebugDrawStates::_draw_log = !DebugDrawStates::_draw_log;
         if (DebugDrawStates::_draw_log && !_log_ui)
         {
            _log_ui = std::make_unique<LogUi>();
         }
         else if (_log_ui)
         {
            _log_ui->close();
            _log_ui.reset();
         }
#endif
         break;
      }
      case sf::Keyboard::Key::F6:
      {
         DebugDrawStates::_draw_test_scene = !DebugDrawStates::_draw_test_scene;
         break;
      }
      case sf::Keyboard::Key::F7:
      {
         DebugDrawStates::_draw_physics_config = !DebugDrawStates::_draw_physics_config;
         if (DebugDrawStates::_draw_physics_config && !_physics_ui)
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
      case sf::Keyboard::Key::F8:
      {
         const auto& serializer = EventSerializer::getInstance("player");
         if (serializer)
         {
            if (serializer->isEnabled())
            {
               serializer->stop();
            }
            else
            {
               serializer->start();
            }
         }
         break;
      }
      case sf::Keyboard::Key::F9:
      {
         const auto& serializer = EventSerializer::getInstance("player");
         if (serializer)
         {
            serializer->play();
         }
         break;
      }
      case sf::Keyboard::Key::F12:
      {
         Console::getInstance().toggleActive();
         break;
      }
      case sf::Keyboard::Key::PageUp:
      {
         LevelRegistry::getCurrent()->getLightSystem()->increaseAmbient(0.1f);
         break;
      }
      case sf::Keyboard::Key::PageDown:
      {
         LevelRegistry::getCurrent()->getLightSystem()->decreaseAmbient(0.1f);
         break;
      }
      case sf::Keyboard::Key::L:
      {
         reloadLevel();
         break;
      }
      case sf::Keyboard::Key::M:
      {
         _recording = !_recording;
         break;
      }
      case sf::Keyboard::Key::N:
      {
         nextLevel();
         break;
      }
      case sf::Keyboard::Key::Q:
      {
         shutdown();
         break;
      }
      case sf::Keyboard::Key::R:
      {
         reset();
         break;
      }
      case sf::Keyboard::Key::S:
      {
         takeScreenshot();
         break;
      }
      case sf::Keyboard::Key::V:
      {
         _player->setVisible(!_player->getVisible());
         break;
      }
      case sf::Keyboard::Key::Num1:
      {
         LevelRegistry::getCurrent()->zoomIn();
         break;
      }
      case sf::Keyboard::Key::Num2:
      {
         LevelRegistry::getCurrent()->zoomOut();
         break;
      }
      case sf::Keyboard::Key::Num3:
      {
         LevelRegistry::getCurrent()->zoomReset();
         break;
      }
#endif
   }
}

void Game::processKeyReleasedEvents(const sf::Event::KeyReleased* event)
{
   CameraPanorama::getInstance().processKeyReleasedEvents(event);
}

void Game::processEvents()
{
   while (const auto event = _window->pollEvent())
   {
      processEvent(event.value());
   }

   if (DebugDrawStates::_draw_physics_config)
   {
      _physics_ui->processEvents();
   }

   if (DebugDrawStates::_draw_camera_system)
   {
      _camera_ui->processEvents();
   }

#ifndef DECEPTUS_VRSFML
   if (DebugDrawStates::_draw_log)
   {
      _log_ui->processEvents();
   }
#endif

#ifdef DEVELOPMENT_MODE
   if (_profiling_ui)
   {
      _profiling_ui->processEvents();
      if (!_profiling_ui->isOpen())
      {
         _profiling_ui->close();
         _profiling_ui.reset();
      }
   }
#endif
}
