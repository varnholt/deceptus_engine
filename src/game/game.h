#pragma once

#include "console.h"
#include "constants.h"
#include "eventserializer.h"
#include "forestscene.h"
#include "infolayer.h"
#include "inventorylayer.h"
#include "overlays/controlleroverlay.h"
#include "overlays/rainoverlay.h"

#include "menus/menu.h"

#include "Box2D/Box2D.h"
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <thread>
#include <future>


class Level;
class Player;


class Game
{
public:

   Game() = default;
   virtual ~Game();

   void initialize();
   int loop();
   void processEvents();
   void draw();

   void takeScreenshot();

private:

   struct DrawStates
   {
      bool _draw_test_scene = false;
      bool _draw_console = false;
      bool _draw_debug_info = false;
      bool _draw_controller_overlay = false;
      bool _draw_camera_system = false;
      bool _draw_weather = true;
   };

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
   void updateGameControllerForInventory();
   void updateAtmosphereShader();
   void updateWindowTitle();

   void checkCloseInventory();
   void openInventory();
   void showMainMenu();
   void showPauseMenu();

   void processKeyPressedEvents(const sf::Event& event);
   void processKeyReleasedEvents(const sf::Event& event);

   void toggleFullScreen();
   void togglePause();
   void changeResolution(int32_t w, int32_t h);


private:

   std::shared_ptr<Player> _player;
   std::shared_ptr<sf::RenderWindow> _window;
   std::shared_ptr<sf::RenderTexture> _window_render_texture;
   std::shared_ptr<Level> _level;
   std::unique_ptr<InfoLayer> _info_layer;
   std::unique_ptr<InventoryLayer> _inventory_layer;
   std::unique_ptr<ControllerOverlay> _controller_overlay;

   // temporary here for debugging only
   std::unique_ptr<ForestScene> _test_scene;

   sf::Clock _delta_clock;
   std::atomic<bool> _level_loading_finished = false;
   std::atomic<bool> _level_loading_finished_previous = false; // keep track of level loading in an async manner
   std::future<void> _level_loading_thread;
   bool _stored_position_valid = false;
   sf::Vector2f _stored_position;

   int32_t _fps = 0;
   bool _screenshot = false;
   DrawStates _draw_states;
   sf::Vector2u _render_texture_offset;
   int32_t _death_wait_time_ms = 0;

   bool _recording = false;
   int32_t _recording_counter = 0;
   std::vector<sf::Image> _recording_images;

   EventSerializer _event_serializer;
};

