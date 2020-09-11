#include "game.h"

#include "animationpool.h"
#include "audio.h"
#include "bullethitanimation.h"
#include "callbackmap.h"
#include "camerapane.h"
#include "debugdraw.h"
#include "displaymode.h"
#include "gameconfiguration.h"
#include "gamecontactlistener.h"
#include "gamecontrollerdata.h"
#include "gamecontrollerintegration.h"
#include "gamejoystickmapping.h"
#include "gamestate.h"
#include "globalclock.h"
#include "level.h"
#include "levels.h"
#include "messagebox.h"
#include "luainterface.h"
#include "player.h"
#include "playerinfo.h"
#include "physicsconfiguration.h"
#include "savestate.h"
#include "timer.h"
#include "weapon.h"
#include "weather.h"
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
   mWindow->setMouseCursorVisible(!gameConfig.mFullscreen);

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

   std::cout << "[x] created window render texture: " << textureWidth << " x " << textureHeight << std::endl;

   if (!mLevel)
   {
      std::cerr << "[!] level not initialized" << std::endl;
      return;
   }

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

      gji->getController()->addButtonPressedCallback(
         SDL_CONTROLLER_BUTTON_START,
         [this](){
            showPauseMenu();
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
   if (Menu::getInstance()->getCurrentType() == Menu::MenuType::None)
   {
      Menu::getInstance()->show(Menu::MenuType::Pause);
      GameState::getInstance().enqueuePause();
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Game::loadLevel()
{
   mLevelLoadingFinished = false;

   mLevelLoadingThread = std::async(
      std::launch::async, [this](){

         // pick a level
         auto levels = Levels::getInstance();
         levels.deserializeFromFile();
         auto levelItem = levels.mLevels.at(SaveState::getCurrent().mLevelIndex);

         mLevel.reset();

         // load it
         mLevel = std::make_shared<Level>();
         mLevel->setDescriptionFilename(levelItem.mLevelName);
         mLevel->initialize();
         mLevel->initializeTextures();

         // put the player in there
         mPlayer->setWorld(mLevel->getWorld());
         mPlayer->initializeLevel();

         // jump back to stored position
         if (mStoredPositionValid)
         {
            mPlayer->setBodyViaPixelPosition(mStoredPosition.x, mStoredPosition.y);
            mStoredPositionValid = false;
         }

         mPlayer->updatePlayerPixelRect();

         mLevelLoadingFinished = true;
      }
   );
}


//----------------------------------------------------------------------------------------------------------------------
void Game::nextLevel()
{
   SaveState::getCurrent().mLevelIndex++;

   auto levels = Levels::getInstance();
   if (SaveState::getCurrent().mLevelIndex == levels.mLevels.size())
   {
       SaveState::getCurrent().mLevelIndex = 0;
   }

   loadLevel();
}


//----------------------------------------------------------------------------------------------------------------------
void Game::initialize()
{
   initializeController();

   mPlayer = std::make_shared<Player>();
   mPlayer->initialize();

   // loadLevel();

   mInfoLayer = std::make_unique<InfoLayer>();
   mInventoryLayer = std::make_unique<InventoryLayer>();
   mControllerOverlay = std::make_unique<ControllerOverlay>();
   mTestScene = std::make_unique<ForestScene>();

   CallbackMap::getInstance().addCallback(CallbackMap::CallbackType::EndGame, [this](){mDrawTestScene = true;});

   Audio::getInstance();

   // initially the game should be in main menu and paused
   std::dynamic_pointer_cast<MenuScreenMain>(Menu::getInstance()->getMenuScreen(Menu::MenuType::Main))->setExitCallback(
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

   Timer::add(std::chrono::milliseconds(1000), [this](){updateWindowTitle();}, Timer::Type::Repeated);

   GameState::getInstance().addCallback(
      [this](ExecutionMode current, ExecutionMode previous){
         if (current == ExecutionMode::Paused && previous == ExecutionMode::Running)
         {
            // std::cout << "reset keys pressed" << std::endl;
            mPlayer->getControls().setKeysPressed(0);
            CameraPane::getInstance().updateLookState(Look::LookActive, false);
         }
      }
   );
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

   const auto mapEnabled = DisplayMode::getInstance().isSet(Display::DisplayMap);

   if (mLevelLoadingFinished)
   {
      mLevel->draw(mWindowRenderTexture, mScreenshot);
   }

   mScreenshot = false;

   if (mDrawWeather)
   {
      Weather::getInstance().draw(*mWindowRenderTexture.get());
   }

   if (!mapEnabled)
   {
      mInfoLayer->setLoading(!mLevelLoadingFinished);
      mInfoLayer->draw(*mWindowRenderTexture.get());
   }

   if (mDrawDebugInfo)
   {
      mInfoLayer->drawDebugInfo(*mWindowRenderTexture.get());
   }

   if (mDrawConsole)
   {
      mInfoLayer->drawConsole(*mWindowRenderTexture.get());
   }

   if (mDrawCameraSystem)
   {
      DebugDraw::debugCameraSystem(*mWindowRenderTexture.get());
   }

   if (mDrawControllerOverlay)
   {
      mControllerOverlay->draw(*mWindowRenderTexture.get());
   }

   if (DisplayMode::getInstance().isSet(Display::DisplayInventory))
   {
      mInventoryLayer->draw(*mWindowRenderTexture.get());
   }

   if (mDrawTestScene)
   {
      mTestScene->draw(*mWindowRenderTexture.get());
   }

   Menu::getInstance()->draw(*mWindowRenderTexture.get(), {sf::BlendAlpha});
   MessageBox::draw(*mWindowRenderTexture.get());

   mWindowRenderTexture->display();
   auto windowTextureSprite = sf::Sprite(mWindowRenderTexture->getTexture());

   if (GameConfiguration::getInstance().mFullscreen)
   {
      // scale window texture up to available window size
      const auto scaleX = mWindow->getSize().x / static_cast<float>(mWindowRenderTexture->getSize().x);
      const auto scaleY = mWindow->getSize().y / static_cast<float>(mWindowRenderTexture->getSize().y);
      const auto scaleMin = std::min(static_cast<int32_t>(scaleX), static_cast<int32_t>(scaleY));
      const auto dx = (scaleX - scaleMin) * 0.5f;
      const auto dy = (scaleY - scaleMin) * 0.5f;
      windowTextureSprite.setPosition(mWindowRenderTexture->getSize().x * dx, mWindowRenderTexture->getSize().y * dy);
      windowTextureSprite.scale(static_cast<float>(scaleMin), static_cast<float>(scaleMin));
   }
   else
   {
      windowTextureSprite.setPosition(
         static_cast<float>(mRenderTextureOffset.x),
         static_cast<float>(mRenderTextureOffset.y)
      );
   }

   mWindow->draw(windowTextureSprite);

   mWindow->popGLStates();

   mWindow->display();

   if (mRecording)
   {
      const auto image = windowTextureSprite.getTexture()->copyToImage();

      std::thread record([this, image](){
            std::ostringstream num;
            num << std::setfill('0') << std::setw(5) << mRecordingCounter++;
            image.saveToFile(num.str() + ".bmp");
         }
      );

      record.detach();
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Game::updateGameController()
{
   auto gji = GameControllerIntegration::getInstance(0);
   if (gji != nullptr)
   {
      gji->getController()->update();
      mPlayer->getControls().setJoystickInfo(gji->getController()->getInfo());
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Game::updateGameControllerForGame()
{
  auto gji = GameControllerIntegration::getInstance(0);
  if (gji != nullptr)
  {
     auto info = gji->getController()->getInfo();
     mPlayer->getControls().setJoystickInfo(info);
     GameControllerData::getInstance().setJoystickInfo(info);
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
void Game::updateGameState(const sf::Time& dt)
{
   // check if player is dead
   auto deathReason = mPlayer->checkDead();
   if (!mPlayer->isDead() && deathReason != DeathReason::None)
   {
      mDeathWaitTimeMs = 0;
      mLevel->resetDeathShader();

      switch (deathReason)
      {
         case DeathReason::TouchesDeadly:
         {
            std::cout << "[i] dead: touched something deadly" << std::endl;
            break;
         }
         case DeathReason::TooFast:
         {
            std::cout << "[i] dead: too fast" << std::endl;
            break;
         }
         case DeathReason::OutOfHealth:
         {
            std::cout << "[i] dead: out of health" << std::endl;
            break;
         }
         case DeathReason::None:
         {
            break;
         }
      }

      mPlayer->die();
   }

   if (mPlayer->isDead())
   {
      mDeathWaitTimeMs += dt.asMilliseconds();
      if (mDeathWaitTimeMs > 2500)
      {
         // std::cout << "reload" << std::endl;
         SaveState::deserializeFromFile();
         mPlayer->reset();
         loadLevel();
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Game::update()
{
   const auto dt = mDeltaClock.getElapsedTime();
   mDeltaClock.restart();

   Audio::getInstance()->updateMusic();

   // reload the level when the save state has been invalidated
   if (SaveState::getCurrent().mLoadLevelRequested)
   {
      SaveState::getCurrent().mLoadLevelRequested = false;
      loadLevel();
   }

   if (GameState::getInstance().getMode() == ExecutionMode::Paused)
   {
      updateGameController();
      updateGameControllerForInventory();
      mInventoryLayer->update(dt);

      // this is not beautiful. simplify!
      if (DisplayMode::getInstance().isSet(Display::DisplayMap))
      {
         CameraPane::getInstance().update();
      }
   }
   else if (GameState::getInstance().getMode() == ExecutionMode::Running)
   {
      Timer::update();

      if (mLevelLoadingFinished)
      {
         AnimationPool::getInstance().updateAnimations(dt);
         Bullet::updateHitAnimations(dt);
         updateGameController();
         updateGameControllerForGame();
         mLevel->update(dt);
         mPlayer->update(dt);

         if (mDrawTestScene)
         {
            mTestScene->update(dt);
         }

         if (mDrawWeather)
         {
            Weather::getInstance().update(dt);
         }

         // this might trigger level-reloading, so this ought to be the last drawing call in the loop
         updateGameState(dt);
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
   mPlayer->reset();
}


//----------------------------------------------------------------------------------------------------------------------
void Game::checkCloseInventory()
{
  if (DisplayMode::getInstance().isSet(Display::DisplayInventory))
  {
     GameState::getInstance().enqueueResume();
     DisplayMode::getInstance().enqueueUnset(Display::DisplayInventory);
  }
}


//----------------------------------------------------------------------------------------------------------------------
void Game::openInventory()
{
   if (GameState::getInstance().getMode() == ExecutionMode::Running)
   {
      GameState::getInstance().enqueuePause();
      DisplayMode::getInstance().enqueueSet(Display::DisplayInventory);
      mInventoryLayer->setActive(true);
   }
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

    if (mLevel)
    {
       mLevel->createViews();
    }
}


//----------------------------------------------------------------------------------------------------------------------
void Game::takeScreenshot()
{
   mScreenshot = true;
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
      if (event.key.code == sf::Keyboard::F12)
      {
         mDrawConsole = !mDrawConsole;
         Console::getInstance().setActive(mDrawConsole);
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

   switch (event.key.code)
   {
      case sf::Keyboard::Num0:
      {
         Audio::getInstance()->playSample("powerup.wav");

         if (SaveState::getPlayerInfo().mExtraTable.mSkills.mSkills & ExtraSkill::SkillClimb)
         {
            SaveState::getPlayerInfo().mExtraTable.mSkills.mSkills &= ~ ExtraSkill::SkillClimb;
         }
         else
         {
            SaveState::getPlayerInfo().mExtraTable.mSkills.mSkills |= ExtraSkill::SkillClimb;
         }
         break;
      }
      case sf::Keyboard::F1:
      {
         DisplayMode::getInstance().enqueueToggle(Display::DisplayDebug);
         break;
      }
      case sf::Keyboard::F2:
      {
         mDrawControllerOverlay = !mDrawControllerOverlay;
         break;
      }
      case sf::Keyboard::F3:
      {
         mDrawCameraSystem = !mDrawCameraSystem;
         break;
      }
      case sf::Keyboard::F4:
      {
         mDrawDebugInfo = !mDrawDebugInfo;
         break;
      }
      case sf::Keyboard::F5:
      {
         mDrawWeather = !mDrawWeather;
         break;
      }
      case sf::Keyboard::F6:
      {
         mDrawTestScene = ! mDrawTestScene;
         break;
      }
      case sf::Keyboard::F12:
      {
         mDrawConsole = !mDrawConsole;
         Console::getInstance().setActive(mDrawConsole);
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
      case sf::Keyboard::L:
      {
         if (mLevelLoadingFinished)
         {
            mStoredPositionValid = true;
            mStoredPosition = mPlayer->getPixelPositionf();
            loadLevel();
         }
         break;
      }
      case sf::Keyboard::M:
      {
         mRecording = !mRecording;
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
         mPlayer->setVisible(!mPlayer->getVisible());
         break;
      }
      case sf::Keyboard::LShift:
      {
         CameraPane::getInstance().updateLookState(Look::LookActive, true);
         break;
      }
      case sf::Keyboard::Left:
      {
         mInventoryLayer->left();
         CameraPane::getInstance().updateLookState(Look::LookLeft, true);
         break;
      }
      case sf::Keyboard::Right:
      {
         mInventoryLayer->right();
         CameraPane::getInstance().updateLookState(Look::LookRight, true);
         break;
      }
      case sf::Keyboard::Return:
      {
         checkCloseInventory();
         break;
      }
      case sf::Keyboard::Up:
      {
         CameraPane::getInstance().updateLookState(Look::LookUp, true);
         break;
      }
      case sf::Keyboard::Down:
      {
         CameraPane::getInstance().updateLookState(Look::LookDown, true);
         break;
      }
      case sf::Keyboard::Tab:
      {
         GameState::getInstance().enqueueTogglePauseResume();
         DisplayMode::getInstance().enqueueToggle(Display::DisplayMap);
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
         CameraPane::getInstance().updateLookState(Look::LookActive, false);
         break;
      }
      case sf::Keyboard::Left:
      {
         CameraPane::getInstance().updateLookState(Look::LookLeft, false);
         break;
      }
      case sf::Keyboard::Right:
      {
         CameraPane::getInstance().updateLookState(Look::LookRight, false);
         break;
      }
      case sf::Keyboard::Up:
      {
         CameraPane::getInstance().updateLookState(Look::LookUp, false);
         break;
      }
      case sf::Keyboard::Down:
      {
         CameraPane::getInstance().updateLookState(Look::LookDown, false);
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
               mPlayer->getControls().keyboardKeyPressed(event.key.code);
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
            mPlayer->getControls().keyboardKeyReleased(event.key.code);
         }

         processKeyReleasedEvents(event);
      }

      else if (event.type == sf::Event::TextEntered)
      {
         if (Console::getInstance().isActive())
         {
            auto unicode = event.text.unicode;

            if (unicode > 0x1F && unicode < 0x80)
            {
               Console::getInstance().append(static_cast<char>(unicode));
            }
         }
      }
   }
}


