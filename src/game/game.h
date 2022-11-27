#pragma once

#include "console.h"
#include "constants.h"
#include "drawstates.h"
#include "eventserializer.h"
#include "forestscene.h"
#include "infolayer.h"
#include "ingamemenu.h"
#include "menus/menu.h"
#include "overlays/controlleroverlay.h"
#include "overlays/rainoverlay.h"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include "Box2D/Box2D.h"

#include <future>
#include <thread>

class Level;
class Player;
struct ScreenTransition;

class Game
{
public:
   Game() = default;
   virtual ~Game();

   void initialize();
   int32_t loop();
   void processEvents();
   void draw();

   void takeScreenshot();

private:
   void processEvent(const sf::Event& event);

   void initializeWindow();
   void initializeController();

   void drawLevel();
   void loadLevel();
   void nextLevel();

   void reset();
   void resetAfterDeath(const sf::Time& dt);

   void update();
   void updateGameState(const sf::Time& dt);
   void updateGameController();
   void updateGameControllerForGame();
   void updateAtmosphereShader();
   void updateWindowTitle();

   void showMainMenu();
   void showPauseMenu();

   void processKeyPressedEvents(const sf::Event& event);
   void processKeyReleasedEvents(const sf::Event& event);

   void toggleFullScreen();
   void togglePause();
   void changeResolution(int32_t w, int32_t h);
   void goToLastCheckpoint();
   void menuLoadRequest();
   std::unique_ptr<ScreenTransition> makeFadeOutFadeIn();

   std::shared_ptr<sf::RenderWindow> _window;
   std::shared_ptr<sf::RenderTexture> _window_render_texture;
   std::shared_ptr<Player> _player;
   std::shared_ptr<Level> _level;
   std::unique_ptr<InfoLayer> _info_layer;
   std::unique_ptr<InGameMenu> _ingame_menu;
   std::unique_ptr<ControllerOverlay> _controller_overlay;

   // temporarily here for debugging only
   std::unique_ptr<ForestScene> _test_scene;

   sf::Clock _delta_clock;
   std::atomic<bool> _level_loading_finished = false;
   std::atomic<bool> _level_loading_finished_previous = false;  // keep track of level loading in an async manner
   std::future<void> _level_loading_thread;
   bool _restore_previous_position = false;
   sf::Vector2f _stored_position;

   int32_t _fps = 0;
   bool _screenshot = false;
   sf::Vector2u _render_texture_offset;
   int32_t _death_wait_time_ms = 0;

   bool _recording = false;
   int32_t _recording_counter = 0;
   std::vector<sf::Image> _recording_images;
};
