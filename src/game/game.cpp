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
#include "menus/menuscreenvideo.h"

#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>
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
   // https://en.sfml-dev.org/forums/index.php?topic=14130.0
   sf::ContextSettings settings;
   settings.stencilBits = 8;

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
      settings
    );

   mWindow->setVerticalSyncEnabled(true);
   mWindow->setFramerateLimit(60);
   mWindow->setKeyRepeatEnabled(false);
   mWindow->setMouseCursorVisible(false);

   // reset render textures if needed
   if (mWindowRenderTexture != nullptr)
   {
      mWindowRenderTexture.reset();
   }

   if (mWindowRenderTexture != nullptr)
   {
      mWindowRenderTexture.reset();
   }

   if (mLevelBackgroundRenderTexture != nullptr)
   {
      mLevelBackgroundRenderTexture.reset();
   }

   if (mAtmosphereRenderTexture != nullptr)
   {
     mAtmosphereRenderTexture.reset();
   }

   // this the render texture size derived from the window dimensions. as opposed to the window
   // dimensions this one takes the view dimensions into regard and preserves an integer multiplier
   const auto ratioWidth = gameConfig.mVideoModeWidth / gameConfig.mViewWidth;
   const auto ratioHeight = gameConfig.mVideoModeHeight / gameConfig.mViewHeight;
   const auto sizeRatio = std::min(ratioWidth, ratioHeight);
   mViewToTextureScale = 1.0f / sizeRatio;

   int32_t textureWidth = sizeRatio * gameConfig.mViewWidth;
   int32_t textureHeight = sizeRatio * gameConfig.mViewHeight;

   mRenderTextureOffset.x = static_cast<uint32_t>((gameConfig.mVideoModeWidth - textureWidth) / 2);
   mRenderTextureOffset.y = static_cast<uint32_t>((gameConfig.mVideoModeHeight - textureHeight) / 2);

   mWindowRenderTexture = std::make_shared<sf::RenderTexture>();
   mWindowRenderTexture->create(
      static_cast<uint32_t>(textureWidth),
      static_cast<uint32_t>(textureHeight)
   );

   mLevelBackgroundRenderTexture = std::make_shared<sf::RenderTexture>();
   mLevelBackgroundRenderTexture->create(
      static_cast<uint32_t>(textureWidth),
      static_cast<uint32_t>(textureHeight)
   );

   mLevelRenderTexture = std::make_shared<sf::RenderTexture>();
   mLevelRenderTexture->create(
      static_cast<uint32_t>(textureWidth),
      static_cast<uint32_t>(textureHeight)
   );

   mAtmosphereRenderTexture = std::make_shared<sf::RenderTexture>();
   mAtmosphereRenderTexture->create(
      static_cast<uint32_t>(textureWidth),
      static_cast<uint32_t>(textureHeight)
   );

   // must be reloaded when the physics texture is reset
   initializeAtmosphereShader();

   mDebugDraw->setRenderTarget(mWindowRenderTexture);
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
   Menu::getInstance().show(Menu::MenuType::Main);
   GameState::getInstance().enqueuePause();
}


//----------------------------------------------------------------------------------------------------------------------
void Game::initialize()
{
  initializeController();

  auto levels = Levels::getInstance();
  levels.deserializeFromFile();
  auto levelOne = levels.mLevels.at(0);

  mLevel = std::make_shared<Level>();
  mLevel->setDescriptionFilename(levelOne.mLevelName);

  mPlayer = std::make_shared<Player>();

  mDebugDraw = std::make_unique<DebugDraw>();
  mDebugDraw->setRenderTarget(mWindowRenderTexture);

  mInfoLayer = std::make_unique<InfoLayer>();
  mInventoryLayer = std::make_unique<InventoryLayer>();

  AnimationPool::getInstance().initialize();

  mLevel->initialize();
  mPlayer->setWorld(mLevel->getWorld());
  mPlayer->initialize();

  Audio::getInstance();

  // initially the game should be in main menu and paused
  std::dynamic_pointer_cast<MenuScreenMain>(Menu::getInstance().getMenuScreen(Menu::MenuType::Main))->setExitCallback(
     [this](){mWindow->close();}
  );

  std::dynamic_pointer_cast<MenuScreenVideo>(Menu::getInstance().getMenuScreen(Menu::MenuType::Video))->setFullscreenCallback(
     [this](){toggleFullScreen();}
  );

  std::dynamic_pointer_cast<MenuScreenVideo>(Menu::getInstance().getMenuScreen(Menu::MenuType::Video))->setResolutionCallback(
     [this](int32_t w, int32_t h){changeResolution(w, h);}
  );

  initializeWindow();

  showMainMenu();

  Timer::add(std::chrono::milliseconds(1000), [this](){updateWindowTitle();}, Timer::Type::Repetitive);
}



//----------------------------------------------------------------------------------------------------------------------
void Game::takeScreenshot(const std::string& basename, sf::RenderTexture& texture)
{
   std::ostringstream ss;
   ss << basename << "_" << std::setw(2) << std::setfill('0') << mScreenshotCounters[basename] << ".png";
   mScreenshotCounters[basename]++;
   texture.getTexture().copyToImage().saveToFile(ss.str());
}


//----------------------------------------------------------------------------------------------------------------------
void Game::drawLevel()
{
   // render atmosphere to atmosphere texture, that texture is used in the shader only
   mAtmosphereRenderTexture->clear();
   mLevel->drawAtmosphereLayer(*mAtmosphereRenderTexture.get());
   mAtmosphereRenderTexture->display();

   if (mScreenshot)
   {
      takeScreenshot("screenshot_atmosphere", *mAtmosphereRenderTexture.get());
   }

   // render layers affected by the atmosphere
   mLevelBackgroundRenderTexture->clear();

   mLevel->updateViews();
   mLevel->drawParallaxMaps(*mLevelBackgroundRenderTexture.get());
   mLevel->drawLayers(*mLevelBackgroundRenderTexture.get(), 0, 15);
   mLevelBackgroundRenderTexture->display();

   if (mScreenshot)
   {
       takeScreenshot("screenshot_level_background", *mLevelBackgroundRenderTexture.get());
   }

   sf::Sprite sprite(mLevelBackgroundRenderTexture->getTexture());

   // draw the atmospheric parts into the level texture
   updateAtmosphereShader();
   mLevelRenderTexture->draw(sprite, &mAtmosphereShader);

   // draw the level layers into the level texture
   mLevel->drawLayers(*mLevelRenderTexture.get(), 16);

   // draw all the other things
   mLevel->drawRaycastLight(*mLevelRenderTexture.get());
   Weapon::drawBulletHits(*mLevelRenderTexture.get());

   sf::View view(sf::FloatRect(0.0f, 0.0f, mLevelRenderTexture->getSize().x, mLevelRenderTexture->getSize().y));
   view.setViewport(sf::FloatRect(0.0f, 0.0f, 1.0f, 1.0f));

   mLevelRenderTexture->setView(view);

   mLevelRenderTexture->display();

   if (mScreenshot)
   {
       takeScreenshot("screenshot_level", *mLevelRenderTexture.get());
   }

   mScreenshot = false;
}


//----------------------------------------------------------------------------------------------------------------------
void Game::initializeAtmosphereShader()
{
  if (!mAtmosphereShader.loadFromFile("data/shaders/water.frag", sf::Shader::Fragment))
  {
    std::cout << "error loading water shader" << std::endl;
    return;
  }

  if (!mAtmosphereDistortionMap.loadFromFile("data/effects/distortion_map.png"))
  {
    std::cout << "error loading distortion map" << std::endl;
    return;
  }

  mAtmosphereDistortionMap.setRepeated(true);
  mAtmosphereDistortionMap.setSmooth(true);

  mAtmosphereShader.setUniform("currentTexture", sf::Shader::CurrentTexture);
  mAtmosphereShader.setUniform("distortionMapTexture", mAtmosphereDistortionMap);
  mAtmosphereShader.setUniform("physicsTexture", mAtmosphereRenderTexture->getTexture());
}


//----------------------------------------------------------------------------------------------------------------------
void Game::updateAtmosphereShader()
{
  float distortionFactor = 0.02f;

  mAtmosphereShader.setUniform("time", GlobalClock::getInstance()->getElapsedTimeInS() * 0.2f);
  mAtmosphereShader.setUniform("distortionFactor", distortionFactor);
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
   drawLevel();

   auto levelTextureSprite = sf::Sprite(mLevelRenderTexture->getTexture());
   levelTextureSprite.scale(mViewToTextureScale, mViewToTextureScale);
   mWindowRenderTexture->draw(levelTextureSprite);

   if (DisplayMode::getInstance().isSet(Display::DisplayDebug))
   {
      debugBodies();
      mLevel->drawStaticChains(mWindow);
   }

   mInfoLayer->draw(*mWindowRenderTexture.get());

   if (DisplayMode::getInstance().isSet(Display::DisplayDebug))
   {
     mInfoLayer->drawDebugInfo(*mWindowRenderTexture.get());
   }

   if (DisplayMode::getInstance().isSet(Display::DisplayInventory))
   {
     mInventoryLayer->draw(*mWindowRenderTexture.get());
   }

   Menu::getInstance().draw(*mWindowRenderTexture.get());
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
      AnimationPool::getInstance().updateAnimations(dt.asSeconds());
      Weapon::updateBulletHitAnimations(dt.asSeconds());
      updateGameController();
      updateGameControllerForGame();
      mLevel->update(dt.asSeconds());
      mPlayer->update(dt);
      updateGameState();
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
         Audio::getInstance()->playSample(Audio::SamplePowerUp);

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
      case sf::Keyboard::P:
      {
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
      case sf::Keyboard::W:
      {
         mAtmosphereEnabled = !mAtmosphereEnabled;
         std::cout << "atmosphere shader is switched " << (mAtmosphereEnabled ? "on" : "off") << std::endl;
         break;
      }
      case sf::Keyboard::Y:
      {
         mDrawPhysics = !mDrawPhysics;
         break;
      }
      case sf::Keyboard::Escape:
      {
         if (Menu::getInstance().getCurrentType() == Menu::MenuType::None)
         {
            showMainMenu();
         }
         // mWindow->close();
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
         Menu::getInstance().keyboardKeyPressed(event.key.code);
         processKeyPressedEvents(event);
      }

      if (event.type == sf::Event::KeyReleased)
      {
         mPlayer->keyboardKeyReleased(event.key.code);
         Menu::getInstance().keyboardKeyReleased(event.key.code);
         processKeyReleasedEvents(event);
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Game::debugBodies()
{
   for (auto joint = mLevel->getWorld()->GetJointList(); joint != nullptr; joint = joint->GetNext())
   {
      auto distanceJoint = dynamic_cast<b2DistanceJoint*>(joint);
      if (distanceJoint != nullptr)
      {
         mDebugDraw->DrawSegment(
            distanceJoint->GetAnchorA(),
            distanceJoint->GetAnchorB(),
            b2Color(1, 1, 0, 1)
         );
      }
   }

   for (
      auto body = mLevel->getWorld()->GetBodyList();
      body != nullptr;
      body = body->GetNext()
   )
   {
      if (
            body->GetType() == b2_dynamicBody
         || body->GetType() == b2_kinematicBody
      )
      {
         auto f = body->GetFixtureList();
         while (f)
         {
            auto next = f->GetNext();
            auto shape = f->GetShape();

            switch (shape->GetType())
            {
               case b2Shape::e_polygon:
               {
                  auto poly = dynamic_cast<b2PolygonShape*>(shape);

                  auto vertexCount = poly->GetVertexCount();
                  auto vertices = new b2Vec2[static_cast<size_t>(vertexCount)];

                  for(auto i = 0; i < vertexCount; i++ )
                  {
                     auto vec2 = poly->GetVertex(i);
                     vertices[i] = vec2;
                     vertices[i].x += body->GetPosition().x;
                     vertices[i].y += body->GetPosition().y;
                  }

                  mDebugDraw->DrawPolygon(
                     vertices,
                     vertexCount,
                     b2Color(1,0,0,1)
                  );

                  delete[] vertices;
                  break;
               }
               case b2Shape::e_circle:
               {
                  b2Vec2 offset;
                  b2CircleShape* circleShape = nullptr;
                  circleShape = dynamic_cast<b2CircleShape*>(f->GetShape());
                  if (circleShape != nullptr)
                  {
                     offset = circleShape->m_p;
                  }

                  mDebugDraw->DrawCircle(
                     body->GetPosition() + offset,
                     shape->m_radius,
                     b2Color(0.4f, 0.4f, 0.4f, 1.0f)
                  );
                  break;
               }
               case b2Shape::e_chain:
               {
                  auto chain = dynamic_cast<b2ChainShape*>(shape);

                  auto vertexCount = chain->m_count;
                  auto vertices = new b2Vec2[static_cast<size_t>(vertexCount)];

                  for(auto i = 0; i < vertexCount; i++ )
                  {
                     auto vec2 = chain->m_vertices[i];
                     vertices[i] = vec2;
                     vertices[i].x += body->GetPosition().x;
                     vertices[i].y += body->GetPosition().y;
                  }

                  mDebugDraw->DrawPolygon(
                     vertices,
                     vertexCount,
                     b2Color(1,0,0,1)
                  );

                  delete[] vertices;
                  break;
               }
               default:
               {
                  break;
               }
            }

            f = next;
         }
      }
      else
      {
         auto vtxIt = mLevel->getPointMap()->find(body);
         auto vtxCountIt = mLevel->getPointSizeMap()->find(body);

         if (
               vtxIt != mLevel->getPointMap()->end()
            && vtxCountIt != mLevel->getPointSizeMap()->end()
         )
         {
            mDebugDraw->DrawPolygon(
               vtxIt->second,
               vtxCountIt->second,
               b2Color(1,0,0)
            );
         }
      }
   }
}
