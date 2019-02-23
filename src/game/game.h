#ifndef GAME_H
#define GAME_H

#include "constants.h"
#include "debugdraw.h"
#include "infolayer.h"
#include "inventorylayer.h"
#include "menus/menu.h"

#include "Box2D/Box2D.h"
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

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
   void initializeAtmosphereShader();
   void initializeController();

   void drawLevel();
   void drawAtmosphere();

   void reset();
   void takeScreenshot(sf::RenderTexture &texture);

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

   void processKeyPressedEvents(const sf::Event& event);
   void processKeyReleasedEvents(const sf::Event& event);

   void toggleFullScreen();
   void changeResolution(int32_t w, int32_t h);

   void debugBodies();


private:

   std::shared_ptr<Player> mPlayer;
   std::shared_ptr<sf::RenderWindow> mWindow;
   std::shared_ptr<sf::RenderTexture> mWindowRenderTexture;
   std::shared_ptr<sf::RenderTexture> mLevelRenderTexture;
   std::shared_ptr<sf::RenderTexture> mAtmosphereRenderTexture;
   std::shared_ptr<Level> mLevel;
   std::unique_ptr<InfoLayer> mInfoLayer;
   std::unique_ptr<InventoryLayer> mInventoryLayer;
   std::unique_ptr<DebugDraw> mDebugDraw;

   sf::Clock mDeltaClock;

   int32_t mFps;
   bool mScreenshot = false;
   bool mDrawPhysics = false;
   int32_t mScreenshotCounter = 0;
   float mViewToTextureScale = 1.0f;
   sf::Vector2u mRenderTextureOffset;

   bool mAtmosphereEnabled = false;
   sf::Shader mAtmosphereShader;
   sf::Texture mAtmosphereDistortionMap;
};

#endif // GAME_H
