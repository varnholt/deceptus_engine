#pragma once

// Include GLEW for OpenGL extensions
#include "opengl/glew.h"

#include "game/audio/audio.h"
#include "game/audio/musicfilenames.h"
#include "game/camera/camerasystemconfigurationui.h"
#include "game/constants.h"
#include "game/debug/console.h"
#include "game/debug/logui.h"
#include "game/ingamemenu/ingamemenu.h"
#include "game/io/eventserializer.h"
#include "game/layers/controlleroverlay.h"
#include "game/layers/infolayer.h"
#include "game/physics/physicsconfigurationui.h"
#include "game/rendering/rendertargets.h"
#include "game/scenes/forestscene.h"
#include "game/sfx/gameaudio.h"
#include "opengl/render3d/menubackgroundscene.h"

#include "box2d/box2d.h"
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <future>

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
   /// \brief initializes game controller integration and pause bindings.
   void initializeController();

   /// \brief draws current level content.
   void drawLevel();
   /// \brief asynchronously loads current save-state level and syncs player/world links.
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
   RenderTargets _render_targets;
   std::shared_ptr<Player> _player;
   std::shared_ptr<Level> _level;
   std::unique_ptr<InfoLayer> _info_layer;
   std::unique_ptr<InGameMenu> _ingame_menu;
   std::unique_ptr<ControllerOverlay> _controller_overlay;
   std::unique_ptr<CameraSystemConfigurationUi> _camera_ui;
   std::unique_ptr<PhysicsConfigurationUi> _physics_ui;
   std::unique_ptr<LogUi> _log_ui;

   // temporarily here for debugging only
   std::unique_ptr<ForestScene> _test_scene;

   std::shared_ptr<EventSerializer> _global_event_serializer;

   // 3D menu background renderer
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
   sf::Vector2u _render_texture_offset;
   int32_t _death_wait_time_ms = 0;

   bool _recording = false;
   int32_t _recording_counter = 0;
   std::vector<sf::Image> _recording_images;
   using GameAudioCallback = std::function<void(GameAudio::SoundEffect)>;
   Audio _audio;
   GameAudioCallback _audio_callback;
};
