#include "level.h"

// game
#include "bouncer.h"
#include "constants.h"
#include "conveyorbelt.h"
#include "door.h"
#include "extraitem.h"
#include "extramanager.h"
#include "gameconfiguration.h"
#include "gamecontactlistener.h"
#include "globalclock.h"
#include "leveldescription.h"
#include "luainterface.h"
#include "meshtools.h"
#include "movingplatform.h"
#include "fixturenode.h"
#include "player.h"
#include "physicsconfiguration.h"
#include "sfmlmath.h"
#include "squaremarcher.h"
#include "tilemap.h"
#include "gamecontrollerintegration.h"
#include "joystick/gamecontroller.h"

// sfml
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include "tmxparser/tmxelement.h"
#include "tmxparser/tmximage.h"
#include "tmxparser/tmxlayer.h"
#include "tmxparser/tmximagelayer.h"
#include "tmxparser/tmxobject.h"
#include "tmxparser/tmxobjectgroup.h"
#include "tmxparser/tmxpolygon.h"
#include "tmxparser/tmxpolyline.h"
#include "tmxparser/tmxproperty.h"
#include "tmxparser/tmxproperties.h"
#include "tmxparser/tmxparser.h"
#include "tmxparser/tmxtileset.h"
#include "tmxparser/tmxtools.h"

// poly2tri
#include "poly2tri/poly2tri.h"
#include "poly2tri/common/shapes.h"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>


Level* Level::sCurrentLevel = nullptr;
const std::string parallaxIdentifier = "parallax_";


//-----------------------------------------------------------------------------
GameControllerInfo Level::getJoystickInfo() const
{
  return mJoystickInfo;
}

void Level::setJoystickInfo(const GameControllerInfo &joystickInfo)
{
  mJoystickInfo = joystickInfo;
}

std::string Level::getDescriptionFilename() const
{
   return mDescriptionFilename;
}

void Level::setDescriptionFilename(const std::string &descriptionFilename)
{
   mDescriptionFilename = descriptionFilename;
}

const Level::Physics& Level::getPhysics() const
{
   return mPhysics;
}

void Level::initializeTextures()
{
   GameConfiguration& gameConfig = GameConfiguration::getInstance();

   // since stencil buffers are used, it is required to enable them explicitly
   sf::ContextSettings contextSettings;
   contextSettings.stencilBits = 8;

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

   mLevelBackgroundRenderTexture = std::make_shared<sf::RenderTexture>();
   mLevelBackgroundRenderTexture->create(
      static_cast<uint32_t>(textureWidth),
      static_cast<uint32_t>(textureHeight)
   );

   mLevelRenderTexture = std::make_shared<sf::RenderTexture>();
   mLevelRenderTexture->create(
      static_cast<uint32_t>(textureWidth),
      static_cast<uint32_t>(textureHeight),
      contextSettings // the lights require stencils
   );

   mAtmosphereRenderTexture = std::make_shared<sf::RenderTexture>();
   mAtmosphereRenderTexture->create(
      static_cast<uint32_t>(textureWidth),
      static_cast<uint32_t>(textureHeight)
   );

   initializeAtmosphereShader();
   initializeGammaShader();
}


Level::Level()
  : GameNode(nullptr),
    mWorld(nullptr)
{
   // init world for this level
   b2Vec2 gravity(0.f, PhysicsConfiguration::getInstance().mGravity);

   mWorld = new b2World(gravity);
   mWorld->SetContactListener(GameContactListener::getInstance());

   sCurrentLevel = this;

   mRaycastLight = std::make_shared<RaycastLight>();
   mStaticLight = std::make_shared<StaticLight>();

   // threaded approach for box2d updates
//   std::thread box2d(
//      [=]()
//      {
//         using namespace std::chrono_literals;
//         while (true)
//         {
//            mWorld->Step(PhysicsConfiguration::getInstance().mTimeStep, 8, 3);
//            std::this_thread::sleep_for(16ms);
//         }
//      }
//   );
//   box2d.detach();
}


//-----------------------------------------------------------------------------
Level::~Level()
{
   delete mWorld;
   mWorld = nullptr;
}


//-----------------------------------------------------------------------------
std::shared_ptr<RaycastLight::LightInstance> Level::deserializeRaycastLight(TmxObject* tmxObject)
{
  auto light = std::make_shared<RaycastLight::LightInstance>();
  light->mWidth = static_cast<int>(tmxObject->mWidth);
  light->mHeight = static_cast<int>(tmxObject->mHeight);
  light->mPosMeters = b2Vec2(
    tmxObject->mX * MPP + (tmxObject->mWidth * 0.5f) * MPP,
    tmxObject->mY * MPP + (tmxObject->mHeight * 0.5f) * MPP
  );

  std::array<int, 4> rgba = {255, 255, 255, 255};
  std::string texture = "data/light/smooth.png";
  if (tmxObject->mProperties != nullptr)
  {
     auto it = tmxObject->mProperties->mMap.find("color");
     if (it != tmxObject->mProperties->mMap.end())
     {
        rgba = TmxTools::color(it->second->mValueStr);
     }

     it = tmxObject->mProperties->mMap.find("texture");
     if (it != tmxObject->mProperties->mMap.end())
     {
        texture = (std::filesystem::path("data/light/") / it->second->mValueStr).string();
     }
  }
  light->mColor.r = rgba[0];
  light->mColor.g = rgba[1];
  light->mColor.b = rgba[2];
  light->mColor.a = rgba[3];
  light->mSprite.setColor(light->mColor);

  light->mTexture.loadFromFile(texture);
  light->mSprite.setTexture(light->mTexture);
  light->mSprite.setTextureRect(
    sf::IntRect(
      0,
      0,
      light->mTexture.getSize().x,
      light->mTexture.getSize().y
    )
  );

  light->mSprite.setPosition(
     sf::Vector2f(
        light->mPosMeters.x * PPM - light->mWidth * 0.5f + light->mOffsetX,
        light->mPosMeters.y * PPM - light->mHeight * 0.5f + light->mOffsetY
     )
  );

  auto scale = static_cast<float>(light->mWidth) / light->mTexture.getSize().x;
  light->mSprite.setScale(scale, scale);

  return light;
}


//-----------------------------------------------------------------------------
std::shared_ptr<StaticLight::LightInstance> Level::deserializeStaticLight(TmxObject* tmxObject, TmxObjectGroup* objectGroup)
{
  // std::cout << "static light: " << objectGroup->mName << " at layer: " << objectGroup->mZ << std::endl;

  auto light = std::make_shared<StaticLight::LightInstance>();
  std::array<int, 4> rgba = {255, 255, 255, 255};
  std::string texture = "data/light/smooth.png";
  auto flickerIntensity = 0.0f;
  auto flickerAlphaAmount = 1.0f;
  auto flickerSpeed = 0.0f;
  if (tmxObject->mProperties != nullptr)
  {
     auto it = tmxObject->mProperties->mMap.find("color");
     if (it != tmxObject->mProperties->mMap.end())
     {
        rgba = TmxTools::color(it->second->mValueStr);
     }

     it = tmxObject->mProperties->mMap.find("texture");
     if (it != tmxObject->mProperties->mMap.end())
     {
        texture = (std::filesystem::path("data/light/") / it->second->mValueStr).string();
     }

     it = tmxObject->mProperties->mMap.find("flicker_intensity");
     if (it != tmxObject->mProperties->mMap.end())
     {
        flickerIntensity = it->second->mValueFloat;
     }

     it = tmxObject->mProperties->mMap.find("flicker_alpha_amount");
     if (it != tmxObject->mProperties->mMap.end())
     {
        flickerAlphaAmount = it->second->mValueFloat;
     }

     it = tmxObject->mProperties->mMap.find("flicker_speed");
     if (it != tmxObject->mProperties->mMap.end())
     {
        flickerSpeed = it->second->mValueFloat;
     }
  }
  light->mColor.r = rgba[0];
  light->mColor.g = rgba[1];
  light->mColor.b = rgba[2];
  light->mColor.a = rgba[3];
  light->mFlickerIntensity = flickerIntensity;
  light->mFlickerAlphaAmount = flickerAlphaAmount;
  light->mFlickerSpeed = flickerSpeed;
  light->mSprite.setColor(light->mColor);
  light->mTexture.loadFromFile(texture);
  light->mSprite.setTexture(light->mTexture);
  light->mSprite.setPosition(tmxObject->mX, tmxObject->mY);
  light->mZ = objectGroup->mZ;

  auto scaleX = tmxObject->mWidth / light->mTexture.getSize().x;
  auto scaleY = tmxObject->mHeight / light->mTexture.getSize().y;
  light->mSprite.scale(scaleX, scaleY);

  return light;
}


//-----------------------------------------------------------------------------
std::shared_ptr<ImageLayer> Level::deserializeImageLayer(TmxElement* element, const std::filesystem::path& levelPath)
{
  std::shared_ptr<ImageLayer> image = std::make_shared<ImageLayer>();
  auto imageLayer = dynamic_cast<TmxImageLayer*>(element);
  image->mZ = imageLayer->mZ;
  image->mTexture.loadFromFile((levelPath / imageLayer->mImage->mSource).string());
  image->mSprite.setPosition(imageLayer->mOffsetX, imageLayer->mOffsetY);
  image->mSprite.setColor(sf::Color(255, 255, 255, static_cast<int32_t>(imageLayer->mOpacity * 255.0f)));
  image->mSprite.setTexture(image->mTexture);

  sf::BlendMode blendMode = sf::BlendAdd;
  if (imageLayer->mProperties != nullptr)
  {
     std::string blendModeStr;
     auto it = imageLayer->mProperties->mMap.find("blendmode");
     if (it != imageLayer->mProperties->mMap.end())
     {
        blendModeStr = it->second->mValueStr;

        if (blendModeStr == "alpha")
        {
           blendMode = sf::BlendAlpha;
        }
        else if (blendModeStr == "multiply")
        {
           blendMode = sf::BlendMultiply;
        }
        else if (blendModeStr == "add")
        {
           blendMode = sf::BlendAdd;
        }
        else if (blendModeStr == "none")
        {
           blendMode = sf::BlendNone;
        }
     }
  }

  image->mBlendMode = blendMode;

  return image;
}


//-----------------------------------------------------------------------------
void Level::deserializeParallaxMap(TmxLayer* layer)
{
   if (layer->mProperties)
   {
      auto parallax = 1.0f;
      auto& map = layer->mProperties->mMap;
      auto itParallaxValue = map.find("parallax");
      if (itParallaxValue != map.end())
      {
        parallax = itParallaxValue->second->mValueFloat;
      }

      auto itParallaxView = map.find("parallax_view");
      if (itParallaxView != map.end())
      {
         int view = itParallaxView->second->mValueInt;
         mParallaxFactor[view] = parallax;
      }
   }
}


//-----------------------------------------------------------------------------
void Level::loadTmx()
{
   auto path = std::filesystem::path(mDescription->mFilename).parent_path();

   sf::Clock elapsed;

   // parse tmx
   std::cout << "[x] parsing tmx... ";

   mTmxParser = std::make_unique<TmxParser>();
   mTmxParser->parse(mDescription->mFilename);

   std::cout << "done within " << elapsed.getElapsedTime().asSeconds() << "s" << std::endl;
   elapsed.restart();

   std::cout << "[x] loading tmx... ";

   auto elements = mTmxParser->getElements();

   for (auto element : elements)
   {
      if (element->mType == TmxElement::TypeLayer)
      {
         auto layer = dynamic_cast<TmxLayer*>(element);
         auto tileset = mTmxParser->getTileSet(layer);

         if (layer->mName == "doors")
         {
            mDoors = Door::load(layer, tileset, path, mWorld);
         }
         else if (layer->mName == "portals")
         {
            mPortals = Portal::load(layer, tileset, path, mWorld);
         }
         else if (layer->mName == "platforms")
         {
            mPlatforms = MovingPlatform::load(layer, tileset, path, mWorld);
         }
         else
         {
            std::shared_ptr<TileMap> tileMap = std::make_shared<TileMap>();
            tileMap->load(layer, tileset, path);

            auto pushTileMap = true;

            if (layer->mName == "physics")
            {
               parsePhysicsLayer(layer, tileset);
               mPhysics.mTileMap = tileMap;
            }
            else if (layer->mName == "extras")
            {
               Player::getPlayer(0)->getExtraManager()->mTilemap = tileMap;
               Player::getPlayer(0)->getExtraManager()->load(layer, tileset);
            }
            else if (layer->mName.compare(0, parallaxIdentifier.length(), parallaxIdentifier) == 0)
            {
               deserializeParallaxMap(layer);
               mParallaxMaps.push_back(tileMap);
               pushTileMap = false;
            }

            parseDynamicPhyicsLayer(layer, tileset);

            if (pushTileMap)
            {
               mTileMaps.push_back(tileMap);
            }
         }
      }

      else if (element->mType == TmxElement::TypeObjectGroup)
      {
         TmxObjectGroup* objectGroup = dynamic_cast<TmxObjectGroup*>(element);
         for (auto object : objectGroup->mObjects)
         {
            TmxObject* tmxObject = object.second;

            if (objectGroup->mName == "portals")
            {
              if (tmxObject->mPolyLine)
              {
                Portal::link(mPortals, tmxObject);
              }
            }
            if (objectGroup->mName == "bouncers")
            {
              Bouncer* bouncer = new Bouncer(
                nullptr,
                mWorld,
                tmxObject->mX,
                tmxObject->mY,
                tmxObject->mWidth,
                tmxObject->mHeight
              );
              bouncer->setZ(objectGroup->mZ);
              mBouncers.push_back(bouncer);
              addDebugRect(bouncer->getBody(), tmxObject->mX, tmxObject->mY, tmxObject->mWidth, tmxObject->mHeight);
            }
            if (objectGroup->mName == "conveyorbelts")
            {
              ConveyorBelt* belt = new ConveyorBelt(
                nullptr,
                mWorld,
                tmxObject->mX,
                tmxObject->mY,
                tmxObject->mWidth,
                tmxObject->mHeight
              );
              auto velocity = 0.0f;
              if (tmxObject->mProperties)
              {
                auto it = tmxObject->mProperties->mMap.find("velocity");
                if (it != tmxObject->mProperties->mMap.end())
                {
                   velocity = it->second->mValueFloat;
                }
              }
              belt->setVelocity(velocity);
              belt->setZ(objectGroup->mZ);
              mConveyorBelts.push_back(belt);
              addDebugRect(belt->getBody(), tmxObject->mX, tmxObject->mY, tmxObject->mWidth, tmxObject->mHeight);
            }
            else if (objectGroup->mName == "platform_paths")
            {
              if (tmxObject->mPolyLine)
              {
                MovingPlatform::link(mPlatforms, tmxObject);
              }
            }

            else if (objectGroup->mName == "lights")
            {
              auto light = deserializeRaycastLight(tmxObject);
              mRaycastLight->mLights.push_back(light);
            }

            else if (objectGroup->mName.compare(0, StaticLight::sLayerName.size(), StaticLight::sLayerName) == 0)
            {
              auto light = deserializeStaticLight(tmxObject, objectGroup);
              mStaticLight->mLights.push_back(light);
            }
         }
      }
      else if (element->mType == TmxElement::TypeImageLayer)
      {
        auto image = deserializeImageLayer(element, path);
        mImageLayers.push_back(image);
      }
   }

   if (!mPhysics.mTileMap)
   {
      printf("no physics layer (called 'physics') found!\n");
      exit(1);
   }

   std::cout << "done within " << elapsed.getElapsedTime().asSeconds() << "s" << std::endl;
}


//-----------------------------------------------------------------------------
bool Level::isLookActive() const
{
   return (mLook & LookActive);
}


//-----------------------------------------------------------------------------
void Level::loadLevel()
{
   auto path = std::filesystem::path(mDescription->mFilename).parent_path();

   // load tmx
   loadTmx();

   sf::Clock elapsed;

   // load static lights
   std::cout << "[x] loading static lights...";
   if (!mStaticLight->mLights.empty())
   {
      mStaticLight->load();
   }
   std::cout << "done within " << elapsed.getElapsedTime().asSeconds() << "s" << std::endl;
   elapsed.restart();

   // load raycast lights
   std::cout << "[x] loading raycast lights...";
   if (!mRaycastLight->mLights.empty())
   {
      mRaycastLight->load();
   }
   std::cout << "done within " << elapsed.getElapsedTime().asSeconds() << "s" << std::endl;
   elapsed.restart();

   // loading ao
   std::cout << "[x] loading ao... ";
   mAo.load(path, std::filesystem::path(mDescription->mFilename).stem().string());

   std::cout << "done within " << elapsed.getElapsedTime().asSeconds() << "s" << std::endl;
}


//-----------------------------------------------------------------------------
void Level::initialize()
{
   createViews();

   mDescription = LevelDescription::load(mDescriptionFilename);

   loadLevel();

   mStartPosition.x = static_cast<float_t>(mDescription->mStartPosition.at(0) * TILE_WIDTH  + PLAYER_ACTUAL_WIDTH / 2);
   mStartPosition.y = static_cast<float_t>(mDescription->mStartPosition.at(1) * TILE_HEIGHT + DIFF_PLAYER_TILE_TO_PHYSICS);

   spawnEnemies();
}


//-----------------------------------------------------------------------------
void Level::spawnEnemies()
{
   for (auto& desc : mDescription->mEnemies)
   {
      std::vector<sf::Vector2f> patrolPath;
      for (auto i = 0; i < desc.mPatrolPath.size(); i+= 2)
      {
         patrolPath.push_back(
            sf::Vector2f(
               static_cast<float_t>(desc.mPatrolPath.at(i)     * TILE_WIDTH + TILE_WIDTH / 2),
               static_cast<float_t>(desc.mPatrolPath.at(i + 1) * TILE_HEIGHT)
            )
         );
      }

      auto enemy = LuaInterface::instance()->addObject(std::string("data/scripts/enemies/") + desc.mScript);

      enemy->mStartPosition = sf::Vector2f(
         static_cast<float_t>(desc.mStartPosition.at(0) * TILE_WIDTH + TILE_WIDTH / 2),
         static_cast<float_t>(desc.mStartPosition.at(1) * TILE_HEIGHT + TILE_HEIGHT / 2)
      );

      enemy->mPatrolPath = patrolPath;
      enemy->initialize();

      mEnemies.push_back(enemy);
   }
}


//-----------------------------------------------------------------------------
void Level::drawStaticChains(std::shared_ptr<sf::RenderWindow>& window)
{
   for (auto path : mPhysics.mOutlines)
   {
      window->draw(&path.at(0), path.size(), sf::LineStrip);
   }
}


//-----------------------------------------------------------------------------
void Level::updateLookVector()
{
  auto speed = 3.0f;
  //  auto maxLength = 100.0f;

  if (isControllerUsed())
  {
    auto axisValues = mJoystickInfo.getAxisValues();
    auto xAxis = GameControllerIntegration::getInstance(0)->getController()->getAxisId(SDL_CONTROLLER_AXIS_RIGHTX);
    auto yAxis = GameControllerIntegration::getInstance(0)->getController()->getAxisId(SDL_CONTROLLER_AXIS_RIGHTY);
    auto x = axisValues[xAxis] / 32768.0f;
    auto y = axisValues[yAxis] / 32768.0f;

    if (fabs(x)> 0.1f || fabs(y)> 0.1f)
    {
      auto w = GameConfiguration::getInstance().mViewWidth * 0.5f;
      auto h = GameConfiguration::getInstance().mViewHeight * 0.5f;
      mLookVector.x = x * w;
      mLookVector.y = y * h;
    }
    else
    {
      mLookVector.x = 0.0f;
      mLookVector.y = 0.0f;
    }
  }
  else if (mLook & LookActive)
  {
    if (mLook & LookUp)
    {
      mLookVector += sf::Vector2f(0.0f, -speed);
    }
    if (mLook & LookDown)
    {
      mLookVector += sf::Vector2f(0.0f, speed);
    }
    if (mLook & LookLeft)
    {
      mLookVector += sf::Vector2f(-speed, 0.0f);
    }
    if (mLook & LookRight)
    {
      mLookVector += sf::Vector2f(speed, 0.0f);
    }

    //    auto len = SfmlMath::lengthSquared(mLookVector);
    //    if (len > maxLength)
    //    {
    //      mLookVector = SfmlMath::normalize(mLookVector);
    //      mLookVector *= maxLength;
    //    }
  }
  else
  {
    mLookVector *= 0.85f;
  }
}


//-----------------------------------------------------------------------------
void Level::updateLookState(Look look, bool enable)
{
  if (enable)
  {
    mLook |= look;
  }
  else
  {
    mLook &= ~look;
  }
}


//-----------------------------------------------------------------------------
std::shared_ptr<sf::View> Level::getLevelView()
{
  return mLevelView;
}


//-----------------------------------------------------------------------------
void Level::createViews()
{
   GameConfiguration& gameConfig = GameConfiguration::getInstance();

   // the view dimensions never change
   mViewWidth = static_cast<float>(gameConfig.mViewWidth);
   mViewHeight = static_cast<float>(gameConfig.mViewHeight);

   mLevelView.reset();
   mLevelView = std::make_shared<sf::View>();
   mLevelView->reset(sf::FloatRect(0.0f, 0.0f, mViewWidth, mViewHeight));
   mLevelView->setViewport(sf::FloatRect(0.0f, 0.0f, 1.0f, 1.0f));

   for (auto i = 0; i < 3; i++)
   {
      mParallaxView[i].reset();
      mParallaxView[i] = std::make_shared<sf::View>();
      mParallaxView[i]->reset(sf::FloatRect(0.0f, 0.0f, mViewWidth, mViewHeight));
      mParallaxView[i]->setViewport(sf::FloatRect(0.0f, 0.0f, 1.0f, 1.0f));
   }

   mMapView = std::make_shared<sf::View>();
   mMapView->setViewport(sf::FloatRect(0.80f, 0.1f, 0.16f, 0.1f));
}


//-----------------------------------------------------------------------------
void Level::updateViews()
{
   auto levelViewX = Player::getPlayer(0)->getPixelPosition().x - mViewWidth / 2.0f + mLookVector.x;
   auto levelViewY = Player::getPlayer(0)->getPixelPosition().y - mViewHeight / 1.5f + mLookVector.y;

   /*
   if (levelViewX < 0)
      levelViewX = 0;
   if (levelViewY < 0)
      levelViewY = 0;

   auto levelWidth  = getSize().x;
   auto levelHeight = getSize().y;

   if (levelViewX > levelWidth - mViewWidth)
      levelViewX = levelWidth - mViewWidth;
   if (levelViewY > levelHeight - mViewHeight)
      levelViewY = levelHeight - mViewHeight;
   */

   mLevelView->reset(sf::FloatRect(levelViewX, levelViewY, mViewWidth, mViewHeight));

   for (auto i = 0; i < 3; i++)
   {
      mParallaxView[i]->reset(
         sf::FloatRect(
            levelViewX * mParallaxFactor[i],
            levelViewY * mParallaxFactor[i],
            mViewWidth,
            mViewHeight
         )
      );
   }

   mMapView->reset(
     sf::FloatRect(
        levelViewX,
        levelViewY,
        mViewWidth * 3.0f,
        mViewHeight * 3.0f
      )
   );
}


//-----------------------------------------------------------------------------
void Level::drawMap(sf::RenderTarget& target)
{
  target.setView(*mMapView);
  for (auto z = 0; z < 50; z++)
  {
     for (auto& tileMap : mTileMaps)
     {
        if (tileMap->getZ() == z)
        {
           target.draw(*tileMap);
        }
     }
  }
}


//-----------------------------------------------------------------------------
void Level::drawRaycastLight(sf::RenderTarget& target)
{
  target.setView(*mLevelView);
  mRaycastLight->draw(target, {});
}


//-----------------------------------------------------------------------------
void Level::drawParallaxMaps(sf::RenderTarget& target)
{
  for (auto i = 0u; i < mParallaxMaps.size(); i++)
  {
     target.setView(*mParallaxView[i]);
     target.draw(*mParallaxMaps[i]);
  }
}


//-----------------------------------------------------------------------------
void Level::drawLayers(sf::RenderTarget& target, int from, int to)
{
   target.setView(*mLevelView);

   for (auto z = from; z <= to; z++)
   {
      mStaticLight->drawToZ(target, {}, z);

      for (auto& tileMap : mTileMaps)
      {
         if (tileMap->getZ() == z)
         {
            target.draw(*tileMap);
         }
      }

      for (auto& platform : mPlatforms)
      {
         if (platform->getZ() == z)
         {
            platform->draw(target);
         }
      }

      for (auto& door : mDoors)
      {
         if (door->getZ() == z)
         {
            door->draw(target);
         }
      }

      for (auto& portal : mPortals)
      {
         if (portal->getZ() == z)
         {
            portal->draw(target);
         }
      }

      for (auto& bouncer : mBouncers)
      {
         if (bouncer->getZ() == z)
         {
            bouncer->draw(target);
         }
      }

      if (z == 11)
      {
         mAo.draw(target);

         for (auto& enemy : mEnemies)
         {
            enemy->draw(target);
         }

         Player::getPlayer(0)->draw(target);
      }

      for_each(std::begin(mImageLayers), std::end(mImageLayers), [&](auto& layer)
      {
         if (layer->mZ == z)
         {
            target.draw(layer->mSprite, {layer->mBlendMode});
         }
      });
   }
}


//-----------------------------------------------------------------------------
void Level::drawAtmosphereLayer(sf::RenderTarget& target)
{
  updateViews();

  mPhysics.mTileMap->setVisible(true);

  target.setView(*mLevelView);
  target.draw(*mPhysics.mTileMap);

  mPhysics.mTileMap->setVisible(false);
}


//----------------------------------------------------------------------------------------------------------------------
void Level::initializeAtmosphereShader()
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
void Level::initializeGammaShader()
{
   if (!mGammaShader.loadFromFile("data/shaders/brightness.frag", sf::Shader::Fragment))
   {
      std::cout << "error loading gamma shader" << std::endl;
      return;
   }

   mGammaShader.setUniform("texture", mLevelRenderTexture->getTexture());
}


//----------------------------------------------------------------------------------------------------------------------
void Level::updateGammaShader()
{
   float brightnessLevel = 0.5f;
   float gamma = 2.2f - (GameConfiguration::getInstance().mBrightness - 0.5f);
   mGammaShader.setUniform("gamma", gamma);
}


//----------------------------------------------------------------------------------------------------------------------
void Level::updateAtmosphereShader()
{
  float distortionFactor = 0.02f;

  mAtmosphereShader.setUniform("time", GlobalClock::getInstance()->getElapsedTimeInS() * 0.2f);
  mAtmosphereShader.setUniform("distortionFactor", distortionFactor);
}


//----------------------------------------------------------------------------------------------------------------------
void Level::takeScreenshot(const std::string& basename, sf::RenderTexture& texture)
{
   std::ostringstream ss;
   ss << basename << "_" << std::setw(2) << std::setfill('0') << mScreenshotCounters[basename] << ".png";
   mScreenshotCounters[basename]++;
   texture.getTexture().copyToImage().saveToFile(ss.str());
}



//-----------------------------------------------------------------------------
void Level::draw(
   const std::shared_ptr<sf::RenderTexture>& window,
   bool screenshot
)
{
   // render atmosphere to atmosphere texture, that texture is used in the shader only
   mAtmosphereRenderTexture->clear();
   drawAtmosphereLayer(*mAtmosphereRenderTexture.get());
   mAtmosphereRenderTexture->display();

   if (screenshot)
   {
      takeScreenshot("screenshot_atmosphere", *mAtmosphereRenderTexture.get());
   }

   // render layers affected by the atmosphere
   mLevelBackgroundRenderTexture->clear();

   updateViews();
   drawParallaxMaps(*mLevelBackgroundRenderTexture.get());
   drawLayers(*mLevelBackgroundRenderTexture.get(), 0, 15);
   mLevelBackgroundRenderTexture->display();

   if (screenshot)
   {
       takeScreenshot("screenshot_level_background", *mLevelBackgroundRenderTexture.get());
   }

   sf::Sprite sprite(mLevelBackgroundRenderTexture->getTexture());

   // draw the atmospheric parts into the level texture
   updateAtmosphereShader();
   mLevelRenderTexture->draw(sprite, &mAtmosphereShader);

   // draw the level layers into the level texture
   drawLayers(*mLevelRenderTexture.get(), 16);

   // draw all the other things
   drawRaycastLight(*mLevelRenderTexture.get());
   Weapon::drawBulletHits(*mLevelRenderTexture.get());

   // display the whole texture
   sf::View view(sf::FloatRect(0.0f, 0.0f, mLevelRenderTexture->getSize().x, mLevelRenderTexture->getSize().y));
   view.setViewport(sf::FloatRect(0.0f, 0.0f, 1.0f, 1.0f));
   mLevelRenderTexture->setView(view);
   mLevelRenderTexture->display();

   if (screenshot)
   {
       takeScreenshot("screenshot_level", *mLevelRenderTexture.get());
   }

   screenshot = false;

   auto levelTextureSprite = sf::Sprite(mLevelRenderTexture->getTexture());
   levelTextureSprite.setPosition(mBoomOffsetX, mBoomOffsetY);
   levelTextureSprite.scale(mViewToTextureScale, mViewToTextureScale);

   updateGammaShader();
   window->draw(levelTextureSprite, &mGammaShader);
}


//-----------------------------------------------------------------------------
sf::Vector2f Level::getSize()
{
   sf::Vector2f vec(500 * TILE_WIDTH, 500 * TILE_WIDTH);
   return vec;
}


//-----------------------------------------------------------------------------
void Level::update(const sf::Time& dt)
{
   // clear conveyor belt state
   ConveyorBelt::update();

   // 80.0f * dt / 60.f
   // http://www.iforce2d.net/b2dtut/worlds
   mWorld->Step(PhysicsConfiguration::getInstance().mTimeStep, 8, 3);

   updateLookVector();
   updateBoom(dt);

   for (auto& tileMap : mTileMaps)
   {
      tileMap->update(dt.asSeconds());
   }

   for (auto& platform : mPlatforms)
   {
      platform->update(dt.asSeconds());
   }

   for (auto& door : mDoors)
   {
      door->update(dt.asSeconds());
   }

   for (auto& bouncer : mBouncers)
   {
      bouncer->update(dt.asSeconds());
   }

   for (auto& portal : mPortals)
   {
      portal->update(dt.asSeconds());
   }

   LuaInterface::instance()->update(dt.asSeconds());

   //mRaycastLight->mLights[0]-> = Player::getPlayer(0)->getBodyPosition();
   mStaticLight->update(GlobalClock::getInstance()->getElapsedTimeInS(), 0.0f, 0.0f);
   mRaycastLight->update(GlobalClock::getInstance()->getElapsedTimeInS(), 0.0f, 0.0f);
}


//----------------------------------------------------------------------------------------------------------------------
void Level::updateBoom(const sf::Time& dt)
{
   if (mBoomIntensity > 0.0f)
   {
      mBoomOffsetX = mViewWidth  * 0.2f * mCameraBoomIntensity * 0.5f * sin(dt.asSeconds() * 71.0f)  * mBoomIntensity;
      mBoomOffsetY = mViewHeight * 0.2f * mCameraBoomIntensity * 0.5f * sin(dt.asSeconds() * 113.0f) * mBoomIntensity;

      // one detionation should take a third of a second
      mBoomIntensity -= 3.0f * dt.asSeconds();
   }
   else
   {
      mBoomIntensity = 0.0f;
      mBoomOffsetX = 0.0f;
      mBoomOffsetY = 0.0f;
   }
}



//-----------------------------------------------------------------------------
b2World *Level::getWorld() const
{
   return mWorld;
}


//-----------------------------------------------------------------------------
void Level::setWorld(b2World *world)
{
    mWorld = world;
}


//-----------------------------------------------------------------------------
b2Vec2 * Level::createShape(
   int tileNumber,
   unsigned int i,
   unsigned int j,
   int tileWidth,
   int tileHeight,
   int horizontalSpan,
   int& polyCount
)
{
   // 1633 => default
   // 1650 => half scale upper
   PhysicsTile tile = (PhysicsTile)tileNumber;

   float offsetX = 0.0f;
   float offsetY = 0.0f;
   float scaleX = 1.0f;
   float scaleY = 1.0f;

   switch (tile)
   {
      case PhysicsTileSolidTop:
      case PhysicsTileOneSidedTop:
      case PhysicsTileDeadlyTop:
         offsetY = 0.0f;
         scaleY  = 0.5f;
         break;
      case PhysicsTileOneSidedBottom:
      case PhysicsTileSolidBottom:
      case PhysicsTileDeadlyBottom:
         offsetY = 0.5f;
         scaleY  = 0.5f;
         break;

      case PhysicsTileSolidLeft:
      case PhysicsTileDeadlyLeft:
         offsetX = 0.0f;
         scaleX  = 0.5f;
         break;

      case PhysicsTileSolidRight:
      case PhysicsTileDeadlyRight:
         offsetX = 0.5f;
         scaleX  = 0.5f;
         break;

      default:
         break;
   }

   // http://forum.cocos2d-objc.org/t/box2d-side-by-side-ground-tiles-flat-ground-yet-blocking-at-small-speed/3058

   polyCount = 4;

   b2Vec2* quad = new b2Vec2[polyCount];
   quad[0].x = (i + offsetX                          ) * tileWidth  / PPM;
   quad[0].y = (j + offsetY                          ) * tileHeight / PPM;

   quad[1].x = (i + offsetX + scaleX + horizontalSpan) * tileWidth  / PPM;
   quad[1].y = (j + offsetY                          ) * tileHeight / PPM;

   quad[2].x = (i + offsetX + scaleX + horizontalSpan) * tileWidth  / PPM;
   quad[2].y = (j + offsetY + scaleY                 ) * tileHeight / PPM;

   quad[3].x = (i + offsetX                          ) * tileWidth  / PPM;
   quad[3].y = (j + offsetY + scaleY                 ) * tileHeight / PPM;

   return quad;
}


//-----------------------------------------------------------------------------
void Level::parsePhysicsLayer(TmxLayer* layer, TmxTileSet* tileSet)
{
   if (layer == nullptr)
   {
     std::cout << "physics tmx layer is a nullptr" << std::endl;
     exit(-1);
   }

   if (tileSet == nullptr)
   {
     std::cout << "physics tmx tileset is a nullptr" << std::endl;
     exit(-1);
   }

   auto tiles  = layer->mData;
   auto width  = layer->mWidth;
   auto height = layer->mHeight;
   auto offsetX = layer->mOffsetX;
   auto offsetY = layer->mOffsetY;

   mPhysics.mMap = new int[width * height];
   mPhysics.mMapWidth = width;
   mPhysics.mMapHeight = height;
   mPhysics.mMapOffsetX = offsetX;
   mPhysics.mMapOffsetY = offsetY;

   for (auto y = 0; y < height; y++)
   {
      for (auto x = 0; x < width; x++)
      {
         // get the current tile number
         auto tileNumber = tiles[y * width + x];
         auto tileRelative = (int32_t)PhysicsTileInvalid;
         if (tileNumber != 0)
         {
            tileRelative = tileNumber - tileSet->mFirstGid;
            mPhysics.mMap[y * width + x] = tileRelative;
         }

          mPhysics.mMap[y * width + x] = tileRelative;
      }
   }

   SquareMarcher m(width, height, mPhysics.mMap, std::vector<int32_t>{PhysicsTileSolidFull} );
   for (auto& path : m.mPaths)
   {
      // path.printPoly();
      std::vector<sf::Vertex> visiblePath;
      for (auto& pos : path.mPolygon)
      {
         sf::Vertex visibleVertex;
         visibleVertex.color = sf::Color(255, 255, 255);
         visibleVertex.position.x = static_cast<float_t>((pos.x + offsetX) * TILE_WIDTH);
         visibleVertex.position.y = static_cast<float_t>((pos.y + offsetY) * TILE_HEIGHT);

         visiblePath.push_back(visibleVertex);
      }
      visiblePath.push_back(visiblePath.at(0));

      std::vector<b2Vec2> chain;
      for (auto& pos : path.mPolygon)
      {
         b2Vec2 chainPos;

         chainPos.Set(
            (pos.x + offsetX)* TILE_WIDTH / PPM,
            (pos.y + offsetY)* TILE_HEIGHT / PPM
         );

         chain.push_back(chainPos);
      }
      chain.push_back(chain.at(0));

      // create 1 body per chain
      b2BodyDef bodyDef;
      bodyDef.position.Set(0, 0);
      bodyDef.type = b2_staticBody;
      b2Body* body = mWorld->CreateBody(&bodyDef);
      b2ChainShape chainShape;
      chainShape.CreateChain(&chain.at(0), static_cast<int32_t>(chain.size()));
      b2FixtureDef fixtureDef;
      fixtureDef.density = 0.0f;
      fixtureDef.friction = 0.2f;
      fixtureDef.shape = &chainShape;
      body->CreateFixture(&fixtureDef);

      mPhysics.mChains.push_back(chain);
      mPhysics.mOutlines.push_back(visiblePath);
   }
}


//-----------------------------------------------------------------------------
void Level::parseDynamicPhyicsLayer(TmxLayer* layer, TmxTileSet* tileSet)
{
   const auto tiles  = layer->mData;
   const auto width  = layer->mWidth;
   const auto height = layer->mHeight;
   const auto offsetX = layer->mOffsetX;
   const auto offsetY = layer->mOffsetY;

   if (tileSet == nullptr)
   {
      // std::cout << "tileset is a nullptr" << std::endl;
      return;
   }

   const auto tileMap = tileSet->mTileMap;

   std::vector<b2Vec2> vertices;
   std::vector<std::vector<int32_t>> faces;

   for (auto y = 0; y < height; y++)
   {
      for (auto x = 0; x < width; x++)
      {
         const auto tileNumber = tiles[y * width + x];
         auto tileRelative = -1;

         if (tileNumber != 0)
         {
            tileRelative = tileNumber - tileSet->mFirstGid;
            auto tileIt = tileMap.find(tileRelative);

            if (tileIt != tileMap.end())
            {
               auto tile = tileIt->second;
               auto objects = tile->mObjectGroup;

               if (objects)
               {
                  for (auto o : objects->mObjects)
                  {
                     auto poly = o.second->mPolygon;
                     auto line = o.second->mPolyLine;

                     std::vector<sf::Vector2f> points;

                     if (poly)
                     {
                        points = poly->mPolyLine;
                     }
                     else if (line)
                     {
                        points = line->mPolyLine;
                     }

                     if (!points.empty())
                     {
                        // std::cout << "poly[" << points.size() << "]: { " << std::endl;
                        std::vector<int32_t> face;
                        for (const auto& p : points)
                        {

                           const auto px = (offsetX + x) * TILE_WIDTH + p.x;
                           const auto py = (offsetY + y) * TILE_HEIGHT + p.y;

                           vertices.push_back(b2Vec2(px, py));
                           face.push_back(static_cast<int32_t>(vertices.size()) - 1);

                           // std::cout << "   " << px << ", " << py << std::endl;
                        }

                        faces.push_back(face);
                        // std::cout << "}" << std::endl;
                     }
                  }
               }
            }
         }
      }
   }

   // https://en.wikipedia.org/wiki/Wavefront_.obj_file
   // Mesh::writeObj("layer_" + layer->mName + ".obj", vertices, faces);

   // weld all the things
   //
   // concept:
   //
   //    If one of the vertices of the neighboring tile touches the current vector (is within epsilon reach)
   //    the follow that path.
   //
   //    If the path can go left or right, always choose the right path.
   //
   //                           *-- -  -
   //     1         23          |
   //     *---------**----------* 4
   //     |         ||          |
   //     *---------**----------* 5
   //     8         76
   //
   //    Create a new path based on the traced vertices. Since a lot of old edges will become obsolete
   //    edges: 1-3, 3-4, 4-5, 5-7, 7-8, 8-1
   //
   //    After creating a closed path, delete all vertices with almost identical x or y positions
   //    edges: 1-4, 4-5, 5-8, 8-1
}


//-----------------------------------------------------------------------------
const sf::Vector2f &Level::getStartPosition() const
{
   return mStartPosition;
}


//-----------------------------------------------------------------------------
Level *Level::getCurrentLevel()
{
   return sCurrentLevel;
}


//-----------------------------------------------------------------------------
void Level::parsePolyline(
   float offsetX,
   float offsetY,
   const std::vector<sf::Vector2f>& poly
)
{
   int positionCount = static_cast<int>(poly.size());
   b2Vec2* points = new b2Vec2[positionCount];

   std::vector<p2t::Point*> polyLine;

   for (int positionIndex = 0; positionIndex < positionCount; positionIndex++)
   {
      sf::Vector2f pos = poly.at(positionIndex) + sf::Vector2f(offsetX, offsetY);

      points[positionIndex].x = pos.x / PPM;
      points[positionIndex].y = pos.y / PPM;

      printf(
         "pos: %d, x: %f, y: %f\n",
         positionIndex,
         pos.x / PPM,
         pos.y / PPM
      );

      fflush(stdout);

      p2t::Point* p = new p2t::Point(pos.x / PPM, pos.y / PPM);
      polyLine.push_back(p);
   }

   p2t::CDT cdt(polyLine);
   cdt.Triangulate();

   std::vector<p2t::Triangle*> triangles = cdt.GetTriangles();

   b2BodyDef bodyDef;
   bodyDef.position.Set(0, 0);
   bodyDef.type = b2_staticBody;
   b2Body* body = mWorld->CreateBody(&bodyDef);

   mPointMap[body]=points;
   mPointCountMap[body]=positionCount;

   for (int i = 0; i < triangles.size(); i++)
   {
      p2t::Point* a = triangles[i]->GetPoint(0);
      p2t::Point* b = triangles[i]->GetPoint(1);
      p2t::Point* c = triangles[i]->GetPoint(2);

      printf(
         "tri %d: %f,%f | %f,%f | %f,%f\n",
         i,
         a->x, a->y, b->x, b->y, c->x, c->y
      );

      fflush(stdout);

      b2Vec2* trianglePoints = new b2Vec2[3];
      trianglePoints[0].x = static_cast<float>(a->x);
      trianglePoints[0].y = static_cast<float>(a->y);
      trianglePoints[1].x = static_cast<float>(b->x);
      trianglePoints[1].y = static_cast<float>(b->y);
      trianglePoints[2].x = static_cast<float>(c->x);
      trianglePoints[2].y = static_cast<float>(c->y);

      b2PolygonShape polygonShape;
      polygonShape.Set(trianglePoints, 3);

      b2FixtureDef fixtureDef;
      fixtureDef.density = 0.0f;
      fixtureDef.shape = &polygonShape;

      body->CreateFixture(&fixtureDef);
   }
}

void Level::addDebugRect(b2Body* body,  float x, float y, float w, float h)
{
  auto points = new b2Vec2[4];
  points[0] = b2Vec2(x * MPP,           y * MPP          );
  points[1] = b2Vec2(x * MPP + w * MPP, y * MPP          );
  points[2] = b2Vec2(x * MPP + w * MPP, y * MPP + h * MPP);
  points[3] = b2Vec2(x * MPP,           y * MPP + h * MPP);

  mPointMap[body] = points;
  mPointCountMap[body] = 4;
}


//-----------------------------------------------------------------------------
bool Level::isTileCombinable(int tileNumber) const
{
   bool combinable = false;
   PhysicsTile tile = (PhysicsTile)tileNumber;

   switch (tile)
   {
      case PhysicsTileSolidFull:
      case PhysicsTileSolidTop:
      case PhysicsTileSolidBottom:
      case PhysicsTileOneSidedFull:
      case PhysicsTileOneSidedTop:
      case PhysicsTileOneSidedBottom:
      case PhysicsTileDeadlyFull:
      case PhysicsTileDeadlyTop:
      case PhysicsTileDeadlyBottom:
         combinable = true;
         break;

      default:
         break;
   }

   return combinable;
}


//-----------------------------------------------------------------------------
bool Level::isTileOneSided(int tileNumber) const
{
   bool oneSided = false;
   PhysicsTile tile = (PhysicsTile)tileNumber;

   switch (tile)
   {
      case PhysicsTileOneSidedFull:
      case PhysicsTileOneSidedTop:
      case PhysicsTileOneSidedBottom:
      case PhysicsTileOneSidedLeft:
      case PhysicsTileOneSidedRight:
      case PhysicsTileOneSidedCornerTopRight:
      case PhysicsTileOneSidedCornerBottomRight:
      case PhysicsTileOneSidedCornerBottomLeft:
      case PhysicsTileOneSidedCornerTopLeft:
         oneSided = true;
         break;

      default:
         break;
   }

   return oneSided;
}


//-----------------------------------------------------------------------------
bool Level::isTileDeadly(int tileNumber) const
{
   bool deadly = false;
   PhysicsTile tile = (PhysicsTile)tileNumber;

   switch (tile)
   {
      case PhysicsTileDeadlyFull:
      case PhysicsTileDeadlyTop:
      case PhysicsTileDeadlyBottom:
      case PhysicsTileDeadlyLeft:
      case PhysicsTileDeadlyRight:
      case PhysicsTileDeadlyCornerTopRight:
      case PhysicsTileDeadlyCornerBottomRight:
      case PhysicsTileDeadlyCornerBottomLeft:
      case PhysicsTileDeadlyCornerTopLeft:
         deadly = true;
         break;

      default:
         break;
   }

   return deadly;
}


//-----------------------------------------------------------------------------
bool Level::isTileSolid(int tileNumber) const
{
   bool solid = true;
   PhysicsTile tile = (PhysicsTile)tileNumber;

   switch (tile)
   {
      case PhysicsTileWaterFull:
      case PhysicsTileWaterTop:
      case PhysicsTileWaterBottom:
      case PhysicsTileWaterLeft:
      case PhysicsTileWaterRight:
      case PhysicsTileWaterCornerTopRight:
      case PhysicsTileWaterCornerBottomRight:
      case PhysicsTileWaterCornerBottomLeft:
      case PhysicsTileWaterCornerTopLeft:
         solid = false;
         break;

      default:
         break;
   }

   return solid;
}


//-----------------------------------------------------------------------------
PhysicsTile Level::Physics::getTileForPosition(const b2Vec2 &playerPos) const
{
   if (mMap == nullptr)
   {
      return PhysicsTileInvalid;
   }

   auto x = playerPos.x - mMapOffsetX;
   auto y = playerPos.y - mMapOffsetY;

   if (x < 0 || x >= mMapWidth)
   {
      return PhysicsTileInvalid;
   }

   if (y < 0 || y >= mMapHeight)
   {
      return PhysicsTileInvalid;
   }

  //   std::cout
  //     << "physics tile position: "
  //     << playerPos.x << " x " << playerPos.y
  //     << ", translated tile position: "
  //     << x << " x " << y
  //     << std::endl;

   auto tx = (int)(x * PPM / TILE_WIDTH);
   auto ty = (int)(y * PPM / TILE_HEIGHT);

   PhysicsTile tile = (PhysicsTile)mMap[ty * mMapWidth + tx];
   return tile;
}


//-----------------------------------------------------------------------------
Portal* Level::getNearbyPortal() const
{
   Portal* nearbyPortal = nullptr;

   for (Portal* portal : mPortals)
   {
      if (portal->isPlayerAtPortal())
      {
         nearbyPortal = portal;
         break;
      }
   }

   return nearbyPortal;
}


//-----------------------------------------------------------------------------
void Level::toggleDoor()
{
   for (auto& door : mDoors)
   {
      door->toggle();
   }
}


//-----------------------------------------------------------------------------
void Level::boom(float /*x*/, float /*y*/, float intensity)
{
   if (intensity > mBoomIntensity)
   {
      mBoomIntensity = intensity;
   }
}


//-----------------------------------------------------------------------------
void Level::reset()
{
   for (auto& door : mDoors)
   {
      door->reset();
   }
}


//-----------------------------------------------------------------------------
std::map<b2Body *, int> *Level::getPointSizeMap()
{
   return &mPointCountMap;
}


//-----------------------------------------------------------------------------
std::map<b2Body *, b2Vec2 *>* Level::getPointMap()
{
   return &mPointMap;
}


//-----------------------------------------------------------------------------
bool Level::isControllerUsed() const
{
  return !mJoystickInfo.getAxisValues().empty();
}
