#include "game.h"

#include "animationpool.h"
#include "audio.h"
#include "bullethitanimation.h"
#include "debugdraw.h"
#include "displaymode.h"
#include "gameconfiguration.h"
#include "gamecontactlistener.h"
#include "gamecontrollerintegration.h"
#include "gamejoystickmapping.h"
#include "gamestate.h"
#include "globalclock.h"
#include "level.h"
#include "levels.h"
#include "messagebox.h"
#include "luainterface.h"
#include "player.h"
#include "physicsconfiguration.h"
#include "timer.h"
#include "weapon.h"
#include "joystick/gamecontroller.h"

#include "menus/menuscreenmain.h"
#include "menus/menuscreenpause.h"
#include "menus/menuscreenvideo.h"

#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <time.h>

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
// |...........||--------------- 480px ---------------||                                      |...........|
// |...........+---------------------------------------+--------------------------------------+...........| 540px 864px
// |...........|#######################################|  -                                   |...........|
// |...........|---------------------------------------|  |                                   |...........|  |     |
// |...........|                                       |  |                                   |...........|  |     |
// |...........|                                       |                                      |...........|  |     |
// |...........|       O /                        +----| 270px                                |...........|  |     |
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
//             |------------------------------------ 960px -----------------------------------|
//                                         R E N D E R T E X T U R E
//
// |------------------------------------------------ 1366px ----------------------------------------------|
//                                                 W I N D O W
//
// window width:           1366px
// window height:           864px
//
// view width:              480px
// view height:             270px
//
// ratio width:            1366px / 480px = 2
// ratio height:            864px / 270px = 3
//
// render texture width:    480 * 2 = 960px
// render texture height:   270 * 2 = 540px

//----------------------------------------------------------------------------------------------------------------------
void Game::initializeWindow()
{
   GameConfiguration& gameConfig = GameConfiguration::getInstance();

   // since stencil buffers are used, it is required to enable them explicitly
   sf::ContextSettings contextSettings;
   contextSettings.stencilBits = 8;

   if (mWindow != nullptr)
   {
      mWindow->close();
      mWindow.reset();
   }

   // the window size is whatever the user sets up or whatever fullscreen resolution the user has
   mWindow = std::make_shared<sf::RenderWindow>(
      sf::VideoMode(
         static_cast<uint32_t>(gameConfig.mVideoModeWidth),
         static_cast<uint32_t>(gameConfig.mVideoModeHeight)
      ),
      GAME_NAME,
      gameConfig.mFullscreen ? sf::Style::Fullscreen : sf::Style::Default,
      contextSettings
    );

   mWindow->setVerticalSyncEnabled(gameConfig.mVSync);
   mWindow->setFramerateLimit(60);
   mWindow->setKeyRepeatEnabled(false);
   mWindow->setMouseCursorVisible(false);

   // reset render textures if needed
   if (mWindowRenderTexture != nullptr)
   {
      mWindowRenderTexture.reset();
   }

   // this the render texture size derived from the window dimensions. as opposed to the window
   // dimensions this one takes the view dimensions into regard and preserves an integer multiplier
   const auto ratioWidth = gameConfig.mVideoModeWidth / gameConfig.mViewWidth;
   const auto ratioHeight = gameConfig.mVideoModeHeight / gameConfig.mViewHeight;
   const auto sizeRatio = std::min(ratioWidth, ratioHeight);

   int32_t textureWidth = sizeRatio * gameConfig.mViewWidth;
   int32_t textureHeight = sizeRatio * gameConfig.mViewHeight;

   mRenderTextureOffset.x = static_cast<uint32_t>((gameConfig.mVideoModeWidth - textureWidth) / 2);
   mRenderTextureOffset.y = static_cast<uint32_t>((gameConfig.mVideoModeHeight - textureHeight) / 2);

   mWindowRenderTexture = std::make_shared<sf::RenderTexture>();
   mWindowRenderTexture->create(
      static_cast<uint32_t>(textureWidth),
      static_cast<uint32_t>(textureHeight)
   );

   mLevel->initializeTextures();
}


//----------------------------------------------------------------------------------------------------------------------
void Game::initializeController()
{
   if (GameControllerIntegration::initializeAll() > 0)
   {
      auto gji = GameControllerIntegration::getInstance(0);

      gji->getController()->addButtonPressedCallback(
         SDL_CONTROLLER_BUTTON_Y,
         [this](){
            openInventory();
         }
      );

      gji->getController()->addButtonPressedCallback(
         SDL_CONTROLLER_BUTTON_A,
         [this](){
            checkCloseInventory();
         }
      );

      gji->getController()->addButtonPressedCallback(
         SDL_CONTROLLER_BUTTON_B,
         [this](){
            checkCloseInventory();
         }
      );
   }
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
   Menu::getInstance()->show(Menu::MenuType::Pause);
   GameState::getInstance().enqueuePause();
}





//----------------------------------------------------------------------------------------------------------------------
void Game::loadLevel()
{
    mLevelLoadingThread = std::async(
        std::launch::async, [this](){
            mLevelLoadingFinished = false;

            // pick a level
            auto levels = Levels::getInstance();
            levels.deserializeFromFile();
            auto levelOne = levels.mLevels.at(mLevelIndex);

            mLevel.reset();

            // load it
            mLevel = std::make_shared<Level>();
            mLevel->setDescriptionFilename(levelOne.mLevelName);
            mLevel->initialize();
            mLevel->initializeTextures();

            // put the player in there
            mPlayer->setWorld(mLevel->getWorld());
            mPlayer->initializeLevel();

            mLevelLoadingFinished = true;

            // jump back to stored position
            if (mStoredPositionValid)
            {
               mPlayer->setBodyViaPixelPosition(mStoredPosition.x, mStoredPosition.y);
               mStoredPositionValid = false;
            }
        }
    );
}


//----------------------------------------------------------------------------------------------------------------------
void Game::initialize()
{
  initializeController();

  mPlayer = std::make_shared<Player>();
  mPlayer->initialize();

  loadLevel();

  mInfoLayer = std::make_unique<InfoLayer>();

  mInventoryLayer = std::make_unique<InventoryLayer>();

  Audio::getInstance();

  // initially the game should be in main menu and paused
  std::dynamic_pointer_cast<MenuScreenMain>(Menu::getInstance()->getMenuScreen(Menu::MenuType::Main))->setExitCallback(
     [this](){mWindow->close();}
  );

  std::dynamic_pointer_cast<MenuScreenPause>(Menu::getInstance()->getMenuScreen(Menu::MenuType::Pause))->setExitCallback(
     [this](){mWindow->close();}
  );

  std::dynamic_pointer_cast<MenuScreenVideo>(Menu::getInstance()->getMenuScreen(Menu::MenuType::Video))->setFullscreenCallback(
     [this](){toggleFullScreen();}
  );

  std::dynamic_pointer_cast<MenuScreenVideo>(Menu::getInstance()->getMenuScreen(Menu::MenuType::Video))->setResolutionCallback(
     [this](int32_t w, int32_t h){changeResolution(w, h);}
  );

  std::dynamic_pointer_cast<MenuScreenVideo>(Menu::getInstance()->getMenuScreen(Menu::MenuType::Video))->setVSyncCallback(
     [this](){
     initializeWindow();
     mLevel->createViews();
    }
  );

  initializeWindow();

  showMainMenu();

  Timer::add(std::chrono::milliseconds(1000), [this](){updateWindowTitle();}, Timer::Type::Repetitive);
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
   mFps++;

   mWindowRenderTexture->clear();
   mWindow->clear(sf::Color::Black);
   mWindow->pushGLStates();

   mWindowRenderTexture->clear();

   if (mLevelLoadingFinished)
   {
       mLevel->draw(mWindowRenderTexture, mScreenshot);
   }

   mScreenshot = false;

   mInfoLayer->setLoading(!mLevelLoadingFinished);
   mInfoLayer->draw(*mWindowRenderTexture.get());

   if (DisplayMode::getInstance().isSet(Display::DisplayDebug))
   {
     mInfoLayer->drawDebugInfo(*mWindowRenderTexture.get());
   }

   if (DisplayMode::getInstance().isSet(Display::DisplayInventory))
   {
     mInventoryLayer->draw(*mWindowRenderTexture.get());
   }

   Menu::getInstance()->draw(*mWindowRenderTexture.get(), {sf::BlendAlpha});
   MessageBox::draw(*mWindowRenderTexture.get());

   mWindowRenderTexture->display();
   auto windowTextureSprite = sf::Sprite(mWindowRenderTexture->getTexture());
   windowTextureSprite.setPosition(mRenderTextureOffset.x, mRenderTextureOffset.y);
   mWindow->draw(windowTextureSprite);

   mWindow->popGLStates();
   mWindow->display();
}


//----------------------------------------------------------------------------------------------------------------------
void Game::updateGameController()
{
   auto gji = GameControllerIntegration::getInstance(0);
   if (gji != nullptr)
   {
      gji->getController()->update();
      mPlayer->setJoystickInfo(gji->getController()->getInfo());
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Game::updateGameControllerForGame()
{
  auto gji = GameControllerIntegration::getInstance(0);
  if (gji != nullptr)
  {
     auto info = gji->getController()->getInfo();
     mPlayer->setJoystickInfo(info);
     mLevel->setJoystickInfo(info);
  }
}


//----------------------------------------------------------------------------------------------------------------------
void Game::updateGameControllerForInventory()
{
  auto gji = GameControllerIntegration::getInstance(0);
  if (gji != nullptr)
  {
     mInventoryLayer->setJoystickInfo(gji->getController()->getInfo());
  }
}


//----------------------------------------------------------------------------------------------------------------------
void Game::updateWindowTitle()
{
   std::ostringstream sStream;
   sStream << GAME_NAME << " - " << mFps << "fps";
   mWindow->setTitle(sStream.str());
   mFps = 0;
}


//----------------------------------------------------------------------------------------------------------------------
void Game::updateGameState()
{
   if (mPlayer->isDead())
   {
      mPlayer->die();
      mPlayer->reset();
      mLevel->reset();
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Game::update()
{
   const auto dt = mDeltaClock.getElapsedTime();
   mDeltaClock.restart();

   if (GameState::getInstance().getMode() == ExecutionMode::Paused)
   {
      updateGameController();
      updateGameControllerForInventory();
      mInventoryLayer->update(dt.asSeconds());
   }
   else if (GameState::getInstance().getMode() == ExecutionMode::Running)
   {
      Timer::update();

      if (mLevelLoadingFinished)
      {
          AnimationPool::getInstance().updateAnimations(dt.asSeconds());
          Bullet::updateHitAnimations(dt.asSeconds());
          updateGameController();
          updateGameControllerForGame();
          mLevel->update(dt);
          mPlayer->update(dt);
          updateGameState();
      }
   }

   GameState::getInstance().sync();
   DisplayMode::getInstance().sync();
}


//----------------------------------------------------------------------------------------------------------------------
int Game::loop()
{
   while (mWindow->isOpen())
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
   mLevel->reset();
   mPlayer->reset();
}


//----------------------------------------------------------------------------------------------------------------------
void Game::checkCloseInventory()
{
  if (DisplayMode::getInstance().isSet(Display::DisplayInventory))
  {
     GameState::getInstance().enqueueTogglePauseResume();
     DisplayMode::getInstance().enqueueUnset(Display::DisplayInventory);
  }
}


//----------------------------------------------------------------------------------------------------------------------
void Game::openInventory()
{
  GameState::getInstance().enqueuePause();
  DisplayMode::getInstance().enqueueSet(Display::DisplayInventory);
  mInventoryLayer->setActive(true);
}


//----------------------------------------------------------------------------------------------------------------------
void Game::toggleFullScreen()
{
    GameConfiguration::getInstance().mFullscreen = !GameConfiguration::getInstance().mFullscreen;
    initializeWindow();
    mLevel->createViews();
}


//----------------------------------------------------------------------------------------------------------------------
void Game::changeResolution(int32_t w, int32_t h)
{
    GameConfiguration::getInstance().mVideoModeWidth = w;
    GameConfiguration::getInstance().mVideoModeHeight = h;
    GameConfiguration::getInstance().serializeToFile();

    initializeWindow();
    mLevel->createViews();
}


//----------------------------------------------------------------------------------------------------------------------
void Game::processKeyPressedEvents(const sf::Event& event)
{
   switch (event.key.code)
   {
      case sf::Keyboard::Num0:
      {
         Audio::getInstance()->playSample("powerup.wav");

         if (mPlayer->mExtraTable->mSkills->mSkills & ExtraSkill::SkillClimb)
         {
            mPlayer->mExtraTable->mSkills->mSkills &= ~ ExtraSkill::SkillClimb;
         }
         else
         {
            mPlayer->mExtraTable->mSkills->mSkills |= ExtraSkill::SkillClimb;
         }
         break;
      }
      case sf::Keyboard::D:
      {
         DisplayMode::getInstance().enqueueToggle(Display::DisplayDebug);
         break;
      }
      case sf::Keyboard::F:
      {
         toggleFullScreen();
         break;
      }
      case sf::Keyboard::I:
      {
         openInventory();
         break;
      }
      case sf::Keyboard::J:
      {
         mPlayer->updateClimb();
         break;
      }
      case sf::Keyboard::L:
      {
         if (mLevelLoadingFinished)
         {
            mStoredPositionValid = true;
            mStoredPosition = mPlayer->getPixelPosition();
            loadLevel();
         }
         break;
      }
      case sf::Keyboard::P:
      {
         if (Menu::getInstance()->getCurrentType() == Menu::MenuType::None)
         {
            showPauseMenu();
         }
         else
         {
            Menu::getInstance()->hide();
         }

         GameState::getInstance().enqueueTogglePauseResume();
         break;
      }
      case sf::Keyboard::R:
      {
         reset();
         break;
      }
      case sf::Keyboard::S:
      {
         mScreenshot = true;
         break;
      }
      case sf::Keyboard::V:
      {
         mPlayer->setVisible(!mPlayer->getVisible());
         break;
      }
      case sf::Keyboard::Y:
      {
         mDrawPhysics = !mDrawPhysics;
         break;
      }
      case sf::Keyboard::Escape:
      {
         if (Menu::getInstance()->getCurrentType() == Menu::MenuType::None)
         {
            showMainMenu();
         }
         break;
      }
      case sf::Keyboard::LShift:
      {
         mLevel->updateLookState(Look::LookActive, true);
         break;
      }
      case sf::Keyboard::Left:
      {
         mInventoryLayer->left();
         mLevel->updateLookState(Look::LookLeft, true);
         break;
      }
      case sf::Keyboard::Right:
      {
         mInventoryLayer->right();
         mLevel->updateLookState(Look::LookRight, true);
         break;
      }
      case sf::Keyboard::Return:
      {
         checkCloseInventory();
         break;
      }
      case sf::Keyboard::Up:
      {
         mLevel->updateLookState(Look::LookUp, true);
         break;
      }
      case sf::Keyboard::Down:
      {
         mLevel->updateLookState(Look::LookDown, true);
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
   switch (event.key.code)
   {
      case sf::Keyboard::LShift:
      {
         mLevel->updateLookState(Look::LookActive, false);
         break;
      }
      case sf::Keyboard::Left:
      {
         mLevel->updateLookState(Look::LookLeft, false);
         break;
      }
      case sf::Keyboard::Right:
      {
         mLevel->updateLookState(Look::LookRight, false);
         break;
      }
      case sf::Keyboard::Up:
      {
         mLevel->updateLookState(Look::LookUp, false);
         break;
      }
      case sf::Keyboard::Down:
      {
         mLevel->updateLookState(Look::LookDown, false);
         break;
      }

      default:
      {
         break;
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Game::processEvents()
{
   sf::Event event;
   while (mWindow->pollEvent(event))
   {
      if (event.type == sf::Event::Closed)
      {
         mWindow->close();
      }

      else if (event.type == sf::Event::KeyPressed)
      {
         if (MessageBox::keyboardKeyPressed(event.key.code))
         {
            // nom nom nom
            return;
         }

         mPlayer->keyboardKeyPressed(event.key.code);
         Menu::getInstance()->keyboardKeyPressed(event.key.code);
         processKeyPressedEvents(event);
      }

      if (event.type == sf::Event::KeyReleased)
      {
         mPlayer->keyboardKeyReleased(event.key.code);
         Menu::getInstance()->keyboardKeyReleased(event.key.code);
         processKeyReleasedEvents(event);
      }
   }
}


