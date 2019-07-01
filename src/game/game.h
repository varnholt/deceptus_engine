#ifndef GAME_H
#define GAME_H

#include "console.h"
#include "constants.h"
#include "infolayer.h"
#include "inventorylayer.h"
#include "menus/menu.h"

#include "Box2D/Box2D.h"
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <thread>
#include <future>

class Level;
class Player;
class RaycastLight;


class Game
{
public:

   Game() = default;

   void initialize();
   int loop();
   void processEvents();
   void draw();


private:

   void initializeWindow();
   void initializeController();

   void drawLevel();
   void loadLevel();

   void reset();

   void update();
   void updateGameState();
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
   void changeResolution(int32_t w, int32_t h);


private:

   std::shared_ptr<Player> mPlayer;
   std::shared_ptr<sf::RenderWindow> mWindow;
   std::shared_ptr<sf::RenderTexture> mWindowRenderTexture;
   std::shared_ptr<Level> mLevel;
   std::unique_ptr<InfoLayer> mInfoLayer;
   std::unique_ptr<InventoryLayer> mInventoryLayer;

   sf::Clock mDeltaClock;
   bool mLevelLoadingFinished = false;
   std::future<void> mLevelLoadingThread;
   bool mStoredPositionValid = false;
   sf::Vector2f mStoredPosition;

   int32_t mFps;
   uint32_t mLevelIndex = 0;
   bool mScreenshot = false;
   bool mDrawPhysics = false;
   sf::Vector2u mRenderTextureOffset;

   bool mRecording = false;
   int32_t mRecordingCounter = 0;
   std::vector<sf::Image> mRecordingImages;
};

#endif // GAME_H
