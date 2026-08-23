#pragma once

// Include GLEW for OpenGL extensions
#include "opengl/glew.h"

#include "game/audio/audio.h"
#include "game/audio/musicfilenames.h"
#include "game/camera/camerasystemconfigurationui.h"
#include "game/constants.h"
#include "game/debug/console.h"
#ifndef DECEPTUS_VRSFML
#include "game/debug/logui.h"
#endif
#ifdef DEVELOPMENT_MODE
#include "game/debug/profilingui.h"
#include "game/debug/rendersectiontimer.h"
#include "game/debug/updatesectiontimer.h"
#endif
#include "game/ingamemenu/ingamemenu.h"
#include "game/io/eventserializer.h"
#include "game/layers/controlleroverlay.h"
#include "game/layers/infolayer.h"
#include "game/physics/fixedtimestep.h"
#include "game/physics/physicsconfigurationui.h"
#include "game/rendering/postprocessingpass.h"
#include "game/rendering/rendertargets.h"
#include "game/scenes/forestscene.h"
#include "game/sfx/gameaudio.h"
#include "opengl/render3d/menubackgroundscene.h"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <future>
#include <optional>
#include "box2d/box2d.h"

class Level;
class Player;
struct ScreenTransition;

/// \brief top-level game runtime that owns windowing, loop, level loading, and event dispatch.
class Game
{
public:
   /// \brief constructs game runtime with default-initialized subsystem handles.
   Game() = default;

   /// \brief unregisters global serializers and releases runtime-owned systems.
   virtual ~Game();

   /// \brief initializes window, subsystems, player, menus, and callbacks.
   void initialize();

   /// \brief runs the main loop until the window is closed.
   /// \return process exit code.
   int32_t loop();

   /// \brief polls and dispatches window events.
   void processEvents();

   /// \brief renders the current frame into offscreen and window targets.
   void draw();

   /// \brief requests a screenshot during the next frame render.
   void takeScreenshot();

private:
   /// \brief shuts down runtime systems before exit.
   void shutdown();

   /// \brief handles one sfml event and routes it to relevant systems.
   /// \param event event to process.
   void processEvent(const sf::Event& event);

   /// \brief creates or recreates window and render textures from configuration.
   void initializeWindow();

   /// \brief creates or recreates the render textures and deferred render targets for the
   ///        configured video mode, without touching the window itself.
   void initializeRenderTargets();

   /// \brief initializes game controller integration and pause bindings.
   void initializeController();

   /// \brief draws current level content.
   void drawLevel();

   /// \brief calls update() and records elapsed time for the profiling ui.
   void timedUpdate();

   /// \brief calls draw() and submits frame timings to the profiling ui.
   void timedDraw();

   /// \brief closes the running update section and opens the next one.
   /// \param name label the elapsed time is added to.
   void markUpdateSection(const char* name);

   /// \brief carries out a level load requested by loadLevel(), if one is pending.
   ///
   /// Called at the top of each frame. Destroys the outgoing level on the thread that owns the
   /// drawing context and strictly before the loader thread starts, and never from inside a call
   /// stack running in that level.
   void processPendingLevelLoad();

   /// \brief requests a load of the current save-state level; the work starts on the next frame.
   /// \param loading_mode loading strategy used by Level initialization.
   void loadLevel(LoadingMode loading_mode = LoadingMode::Standard);

   /// \brief reloads the active level while preserving selected transient state.
   /// \param loading_mode loading strategy used by Level initialization.
   void reloadLevel(LoadingMode loading_mode = LoadingMode::Standard);

   /// \brief advances save-state level index and starts loading the next level.
   void nextLevel();

   /// \brief resets player state using current checkpoint information.
   void reset();

   /// \brief drives death fade transitions and checkpoint reload timing.
   /// \param dt elapsed frame time.
   void resetAfterDeath(const sf::Time& dt);

   /// \brief updates timers, state machines, gameplay, and ui each frame.
   void update();

   /// \brief checks player death reasons and triggers post-death flow.
   /// \param dt elapsed frame time.
   void updateGameState(const sf::Time& dt);

   /// \brief updates controller devices and pushes state into player controls.
   void updateGameController();

   /// \brief forwards controller state to gameplay-specific controller storage.
   void updateGameControllerForGame();

   /// \brief updates atmosphere shader parameters.
   void updateAtmosphereShader();

   /// \brief refreshes window title with runtime fps data.
   void updateWindowTitle();

   /// \brief opens the main menu and switches game state to not running.
   void showMainMenu();

   /// \brief opens the pause menu when pausing is currently allowed.
   void showPauseMenu();

   /// \brief handles gameplay-level key-press shortcuts and actions.
   /// \param event key press event data.
   void processKeyPressedEvents(const sf::Event::KeyPressed* event);

   /// \brief handles gameplay-level key-release actions.
   /// \param event key release event data.
   void processKeyReleasedEvents(const sf::Event::KeyReleased* event);

   /// \brief toggles fullscreen mode and recreates rendering resources.
   void toggleFullScreen();

   /// \brief applies a new window resolution and recreates rendering resources.
   /// \param w target width in pixels.
   /// \param h target height in pixels.
   void changeResolution(int32_t w, int32_t h);

   /// \brief applies the scaling options to the render texture filtering.
   void applyScalingOptions();

   /// \brief takes on a size the window already has, without recreating the window.
   /// \param width new window width in pixels.
   /// \param height new window height in pixels.
   void adoptWindowSize(int32_t width, int32_t height);

#ifdef DECEPTUS_VRSFML
   /// \brief re-fits the render resolution to the current browser viewport (integer multiple of the base
   /// view), recreating rendering resources only when the size actually changes. invoked on browser resize
   /// and fullscreen transitions so the game fills the itch/browser window without fractional scaling.
   void refitToViewport();
#endif

   /// \brief reloads save-state data and loads level at last checkpoint.
   void goToLastCheckpoint();

   /// \brief handles menu-triggered level load request with fade-in transition.
   void menuLoadRequest();

   /// \brief queues looping menu music.
   void playMenuMusic();

   /// \brief queues looping level music.
   void playLevelMusic();

   std::shared_ptr<sf::RenderWindow> _window;
   std::shared_ptr<sf::RenderTexture> _window_render_texture;

   //! \brief owns the render target and blits needed to apply a post processing effect to the frame
   PostProcessingPass _post_processing_pass;
   RenderTargets _render_targets;
   std::shared_ptr<Player> _player;
   std::shared_ptr<Level> _level;

   //! \brief banks frame time so the simulation is stepped at a fixed rate whatever the frame rate
   FixedTimeStep _fixed_time_step;

   //! \brief load requested by loadLevel(), carried out by processPendingLevelLoad() next frame
   std::optional<LoadingMode> _pending_level_load;
   std::unique_ptr<InfoLayer> _info_layer;
   std::unique_ptr<InGameMenu> _ingame_menu;
   std::unique_ptr<ControllerOverlay> _controller_overlay;
   std::unique_ptr<CameraSystemConfigurationUi> _camera_ui;
   std::unique_ptr<PhysicsConfigurationUi> _physics_ui;
#ifndef DECEPTUS_VRSFML
   std::unique_ptr<LogUi> _log_ui;
#endif
#ifdef DEVELOPMENT_MODE
   std::unique_ptr<ProfilingUi> _profiling_ui;
   sf::Time _profiling_update_elapsed;

   //! Level::draw reports its own passes; this covers everything else Game::draw does, so the
   //! sections add up to the measured draw time instead of leaving an unexplained remainder
   RenderSectionTimer _draw_section_timer;

   //! the same accounting for the update half of the frame, which the report only ever showed as a
   //! single number. it accumulates by name rather than by position because the simulation runs a
   //! whole number of fixed steps per frame, so a section can be entered more than once
   UpdateSectionTimer _update_section_timer;
#endif

   std::shared_ptr<EventSerializer> _global_event_serializer;

#ifndef DECEPTUS_VRSFML
   // temporarily here for debugging only
   std::unique_ptr<ForestScene> _test_scene;
#endif

   // 3D menu background renderer (raw OpenGL: GLSL 4.30 on desktop, GLSL ES 3.00 on WebGL2)
   std::unique_ptr<MenuBackgroundScene> _menu_background;

   sf::Clock _delta_clock;
   std::atomic<bool> _level_loading_finished = false;
   std::atomic<bool> _level_loading_finished_previous = false;  // keep track of level loading in an async manner
   std::future<void> _level_loading_thread;
   std::vector<std::function<void()>> _level_loaded_callbacks;
   bool _restore_previous_position = false;
   sf::Vector2f _stored_position;

   int32_t _fps = 0;
   bool _screenshot = false;
   int32_t _death_wait_time_ms = 0;

   bool _recording = false;
   int32_t _recording_counter = 0;
   std::vector<sf::Image> _recording_images;
   using GameAudioCallback = std::function<void(GameAudio::SoundEffect)>;
   Audio _audio;
   GameAudioCallback _audio_callback;
};
