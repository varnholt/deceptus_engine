#include "level.h"

// game
#include "bouncer.h"
#include "camerapane.h"
#include "constants.h"
#include "conveyorbelt.h"
#include "debugdraw.h"
#include "displaymode.h"
#include "door.h"
#include "extraitem.h"
#include "extramanager.h"
#include "fan.h"
#include "gameconfiguration.h"
#include "gamecontactlistener.h"
#include "globalclock.h"
#include "laser.h"
#include "leveldescription.h"
#include "levelmap.h"
#include "luainterface.h"
#include "maptools.h"
#include "meshtools.h"
#include "movingplatform.h"
#include "fixturenode.h"
#include "player.h"
#include "physicsconfiguration.h"
#include "sfmlmath.h"
#include "squaremarcher.h"
#include "tilemap.h"

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
std::string Level::getDescriptionFilename() const
{
   return mDescriptionFilename;
}


//-----------------------------------------------------------------------------
void Level::setDescriptionFilename(const std::string &descriptionFilename)
{
   mDescriptionFilename = descriptionFilename;
}


//-----------------------------------------------------------------------------
const Level::Atmosphere& Level::getPhysics() const
{
   return mAtmosphere;
}


//-----------------------------------------------------------------------------
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

   if (mBlurRenderTexture != nullptr)
   {
      mBlurRenderTexture.reset();
   }

   if (mBlurRenderTextureScaled != nullptr)
   {
      mBlurRenderTextureScaled.reset();
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

   mBlurRenderTexture = std::make_shared<sf::RenderTexture>();
   mBlurRenderTexture->create(
      static_cast<uint32_t>(textureWidth),
      static_cast<uint32_t>(textureHeight)
   );

   mBlurRenderTextureScaled = std::make_shared<sf::RenderTexture>();
   mBlurRenderTextureScaled->create(960, 540);
   mBlurRenderTextureScaled->setSmooth(true);

   // keep track of those textures
   mRenderTextures.clear();
   mRenderTextures.push_back(mLevelRenderTexture);
   mRenderTextures.push_back(mLevelBackgroundRenderTexture);
   mRenderTextures.push_back(mAtmosphereRenderTexture);
   mRenderTextures.push_back(mBlurRenderTexture);
   mRenderTextures.push_back(mBlurRenderTextureScaled);
   // for (const auto& fb : mRenderTextures)
   // {
   //    std::cout << "[x] created render texture: " << fb->getSize().x << " x " << fb->getSize().y << std::endl;
   // }

   initializeAtmosphereShader();
   initializeGammaShader();
   initializeBlurShader();
}


//-----------------------------------------------------------------------------
Level::Level()
  : GameNode(nullptr),
    mWorld(nullptr)
{
   // init world for this level
   b2Vec2 gravity(0.f, PhysicsConfiguration::getInstance().mGravity);

   LuaInterface::instance()->reset();

   mWorld = std::make_shared<b2World>(gravity);

   GameContactListener::getInstance()->reset();
   mWorld->SetContactListener(GameContactListener::getInstance());

   sCurrentLevel = this;

   mRaycastLight = std::make_shared<RaycastLight>();
   mStaticLight = std::make_shared<StaticLight>();

   mMap = std::make_unique<LevelMap>();
}


//-----------------------------------------------------------------------------
Level::~Level()
{
   std::cout << "[x] deleting current level" << std::endl;
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

  std::array<uint8_t, 4> rgba = {255, 255, 255, 255};
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
      static_cast<int32_t>(light->mTexture.getSize().x),
      static_cast<int32_t>(light->mTexture.getSize().y)
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
  std::array<uint8_t, 4> rgba = {255, 255, 255, 255};
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
  image->mSprite.setColor(sf::Color(255, 255, 255, static_cast<uint32_t>(imageLayer->mOpacity * 255.0f)));
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
         mParallaxFactors[view] = parallax;
      }
   }
}


//-----------------------------------------------------------------------------
void Level::loadTmx()
{
   auto path = std::filesystem::path(mDescription->mFilename).parent_path();

   sf::Clock elapsed;

   // parse tmx
   std::cout << "[x] parsing tmx... " << std::endl;

   mTmxParser = std::make_unique<TmxParser>();
   mTmxParser->parse(mDescription->mFilename);

   std::cout << "[x] parsing tmx, done within " << elapsed.getElapsedTime().asSeconds() << "s" << std::endl;
   elapsed.restart();

   std::cout << "[x] loading tmx... " << std::endl;

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
         else if (layer->mName == "lasers")
         {
            mLasers = Laser::load(layer, tileset, path, mWorld);
         }
         else // tile map
         {
            std::shared_ptr<TileMap> tileMap = std::make_shared<TileMap>();
            tileMap->load(layer, tileset, path);

            auto pushTileMap = true;

            if (layer->mName == "fans")
            {
               Fan::load(layer, tileset, mWorld);
            }
            else if (layer->mName == "atmosphere")
            {
               mAtmosphere.mTileMap = tileMap;
               parseAtmosphereLayer(layer, tileset);
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
            else if (layer->mName == "level")
            {
               parsePhysicsTiles(layer, tileset, path);
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

            if (objectGroup->mName == "lasers")
            {
               Laser::addObject(tmxObject);
            }
            else if (objectGroup->mName == "fans")
            {
               Fan::addObject(tmxObject);
            }
            else if (objectGroup->mName == "portals")
            {
               if (tmxObject->mPolyLine)
               {
                  Portal::link(mPortals, tmxObject);
               }
            }
            else if (objectGroup->mName == "bouncers")
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

               addDebugRect(
                  bouncer->getBody(),
                  tmxObject->mX,
                  tmxObject->mY,
                  tmxObject->mWidth,
                  tmxObject->mHeight
               );
            }

            else if (objectGroup->mName == "conveyorbelts")
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

   Laser::merge();
   Fan::merge();

   mMap->loadLevelTexture(path / std::filesystem::path("physics_path_solid.png"));

   if (!mAtmosphere.mTileMap)
   {
      std::cerr << "[E] fatal: no physics layer (called 'physics') found!" << std::endl;
   }

   std::cout << "[x] loading tmx, done within " << elapsed.getElapsedTime().asSeconds() << "s" << std::endl;
}


//-----------------------------------------------------------------------------
BoomEffect& Level::getBoomEffect()
{
    return mBoomEffect;
}


//-----------------------------------------------------------------------------
void Level::load()
{
   auto path = std::filesystem::path(mDescription->mFilename).parent_path();

   // load tmx
   loadTmx();

   sf::Clock elapsed;

   // load static lights
   std::cout << "[x] loading static lights..." << std::endl;
   if (!mStaticLight->mLights.empty())
   {
      mStaticLight->load();
   }
   std::cout << "[x] loading static lights, done within " << elapsed.getElapsedTime().asSeconds() << "s" << std::endl;
   elapsed.restart();

   // load raycast lights
   std::cout << "[x] loading raycast lights..." << std::endl;
   if (!mRaycastLight->mLights.empty())
   {
      mRaycastLight->load();
   }
   std::cout << "[x] loading raycast lights, done within " << elapsed.getElapsedTime().asSeconds() << "s" << std::endl;
   elapsed.restart();

   // loading ao
   std::cout << "[x] loading ao... " << std::endl;
   mAo.load(path, std::filesystem::path(mDescription->mFilename).stem().string());

   std::cout << "[x] loading ao, done within " << elapsed.getElapsedTime().asSeconds() << "s" << std::endl;

   std::cout << "[x] level loading complete" << std::endl;
}


//-----------------------------------------------------------------------------
void Level::initialize()
{
   createViews();

   mDescription = LevelDescription::load(mDescriptionFilename);

   load();

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
      for (auto i = 0u; i < desc.mPatrolPath.size(); i+= 2)
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
void Level::drawStaticChains(sf::RenderTarget& target)
{
   for (auto path : mAtmosphere.mOutlines)
   {
      target.draw(&path.at(0), path.size(), sf::LineStrip);
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
   mMapView->setViewport(sf::FloatRect(0.65f, 0.05f, 0.32f, 0.18f));
}


//-----------------------------------------------------------------------------
void Level::updateViews()
{
   auto& cameraSystem = CameraSystem::getCameraSystem();
   cameraSystem.update(mViewWidth, mViewHeight);

   const auto lookVector = CameraPane::getInstance().getLookVector();
   auto levelViewX = cameraSystem.getX() + lookVector.x;
   auto levelViewY = cameraSystem.getY() + lookVector.y;

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
            levelViewX * mParallaxFactors[i],
            levelViewY * mParallaxFactors[i],
            mViewWidth,
            mViewHeight
         )
      );
   }

   mMapView->reset(
     sf::FloatRect(
        levelViewX,
        levelViewY,
        mViewWidth * 5.0f,
        mViewHeight * 5.0f
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

      for (auto& laser : mLasers)
      {
         if (laser->getZ() == z)
         {
            laser->draw(target);
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

  mAtmosphere.mTileMap->setVisible(true);

  target.setView(*mLevelView);
  target.draw(*mAtmosphere.mTileMap);

  mAtmosphere.mTileMap->setVisible(false);
}


//-----------------------------------------------------------------------------
void Level::drawBlurLayer(sf::RenderTarget& target)
{
  updateViews();

  target.setView(*mLevelView);

  // draw lasers
  for (auto l : mLasers)
  {
     l->draw(target);
  }
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
void Level::initializeBlurShader()
{
   if (!mBlurShader.loadFromFile("data/shaders/blur.frag", sf::Shader::Fragment))
   {
      std::cout << "error loading blur shader" << std::endl;
      return;
   }

   mBlurShader.setUniform("texture", mBlurRenderTexture->getTexture());
}


//----------------------------------------------------------------------------------------------------------------------
void Level::updateGammaShader()
{
   float gamma = 2.2f - (GameConfiguration::getInstance().mBrightness - 0.5f);
   mGammaShader.setUniform("gamma", gamma);
}


//----------------------------------------------------------------------------------------------------------------------
bool Level::isPhysicsPathClear(const sf::Vector2i& a, const sf::Vector2i& b) const
{
   auto blocks = [this](uint32_t x, uint32_t y) -> bool {
      return mPhysics.mPhysicsMap[(mPhysics.mGridWidth * y) + x] == 1;
   };

   return MapTools::lineCollide(a.x, a.y, b.x, b.y, blocks);
}


//----------------------------------------------------------------------------------------------------------------------
void Level::updateAtmosphereShader()
{
  float distortionFactor = 0.02f;

  mAtmosphereShader.setUniform("time", GlobalClock::getInstance()->getElapsedTimeInS() * 0.2f);
  mAtmosphereShader.setUniform("distortionFactor", distortionFactor);
}


//----------------------------------------------------------------------------------------------------------------------
void Level::updateBlurShader()
{
   // that implicitly scales the effect up by 2
   mBlurShader.setUniform("texture_width", 960/2);
   mBlurShader.setUniform("texture_height", 540/2);

   mBlurShader.setUniform("blur_radius", 20.0f);
   mBlurShader.setUniform("add_factor", 1.0f);
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

   // render glowing elements
   mBlurRenderTexture->clear({0, 0, 0, 0});
   drawBlurLayer(*mBlurRenderTexture.get());
   mBlurRenderTexture->display();

   if (screenshot)
   {
      takeScreenshot("screenshot_blur", *mBlurRenderTexture.get());
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

   // draw the atmospheric parts into the level texture
   sf::Sprite backgroundSprite(mLevelBackgroundRenderTexture->getTexture());
   updateAtmosphereShader();
   mLevelRenderTexture->draw(backgroundSprite, &mAtmosphereShader);

   // draw the glowing parts into the level texture
   //
   // simple but slow approach:
   //
   //   sf::Sprite blurSprite(mBlurRenderTexture->getTexture());
   //   updateBlurShader();
   //   sf::RenderStates states;
   //   states.blendMode = sf::BlendAdd;
   //   states.shader = &mBlurShader;
   //   mLevelRenderTexture->draw(blurSprite, states);

   sf::Sprite blurSprite(mBlurRenderTexture->getTexture());
   const auto downScaleX = mBlurRenderTextureScaled->getSize().x / static_cast<float>(mBlurRenderTexture->getSize().x);
   const auto downScaleY = mBlurRenderTextureScaled->getSize().y / static_cast<float>(mBlurRenderTexture->getSize().y);
   blurSprite.scale({downScaleX, downScaleY});

   sf::RenderStates statesShader;
   updateBlurShader();
   statesShader.shader = &mBlurShader;
   mBlurRenderTextureScaled->draw(blurSprite, statesShader);

   sf::Sprite blurScaleSprite(mBlurRenderTextureScaled->getTexture());
   blurScaleSprite.scale(1.0f / downScaleX, 1.0f / downScaleY);
   blurScaleSprite.setTextureRect(
      sf::IntRect(
         0,
         static_cast<int32_t>(blurScaleSprite.getTexture()->getSize().y),
         static_cast<int32_t>(blurScaleSprite.getTexture()->getSize().x),
         static_cast<int32_t>(-blurScaleSprite.getTexture()->getSize().y)
      )
   );

   sf::RenderStates statesAdd;
   statesAdd.blendMode = sf::BlendAdd;
   mLevelRenderTexture->draw(blurScaleSprite, statesAdd);

   // draw the level layers into the level texture
   drawLayers(*mLevelRenderTexture.get(), 16);

   // draw all the other things
   drawRaycastLight(*mLevelRenderTexture.get());
   Weapon::drawBulletHits(*mLevelRenderTexture.get());

   if (DisplayMode::getInstance().isSet(Display::DisplayDebug))
   {
      drawStaticChains(*mLevelRenderTexture.get());
      DebugDraw::debugBodies(*mLevelRenderTexture.get(), this);
      DebugDraw::drawRect(*mLevelRenderTexture.get(), Player::getPlayer(0)->getPlayerRect());
   }

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

   levelTextureSprite.setPosition(mBoomEffect.mBoomOffsetX, mBoomEffect.mBoomOffsetY);

   levelTextureSprite.scale(mViewToTextureScale, mViewToTextureScale);

   updateGammaShader();
   window->draw(levelTextureSprite, &mGammaShader);

   if (DisplayMode::getInstance().isSet(Display::DisplayDebug))
   {
      drawMap(*window.get());
   }

   if (DisplayMode::getInstance().isSet(Display::DisplayMap))
   {
      mMap->draw(*window.get());
   }
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

   CameraPane::getInstance().update();
   mBoomEffect.update(dt);

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

   for (auto& laser : mLasers)
   {
      laser->update(dt.asSeconds());
   }

   LuaInterface::instance()->update(dt.asSeconds());

   mStaticLight->update(GlobalClock::getInstance()->getElapsedTimeInS(), 0.0f, 0.0f);
   mRaycastLight->update(GlobalClock::getInstance()->getElapsedTimeInS(), 0.0f, 0.0f);
}


//-----------------------------------------------------------------------------
const std::shared_ptr<b2World>& Level::getWorld() const
{
   return mWorld;
}


//-----------------------------------------------------------------------------
void Level::setWorld(const std::shared_ptr<b2World>& world)
{
    mWorld = world;
}


//-----------------------------------------------------------------------------
void Level::addPathsToWorld(
   int32_t offsetX,
   int32_t offsetY,
   const std::vector<SquareMarcher::Path>& paths,
   ObjectBehavior behavior
)
{
   sf::Color color;
   switch (behavior)
   {
      case ObjectBehaviorSolid:
         color = sf::Color(255, 255, 255);
         break;
      case ObjectBehaviorDeadly:
         color = sf::Color(255, 0, 0);
         break;
   }

   for (auto& path : paths)
   {
      // just for debugging purposes, this section can be removed later
      {
         // path.printPoly();
         std::vector<sf::Vertex> visiblePath;
         for (auto& pos : path.mScaled)
         {
            sf::Vertex visibleVertex;
            visibleVertex.color = color;
            visibleVertex.position.x = static_cast<float_t>((pos.x + offsetX) * TILE_WIDTH);
            visibleVertex.position.y = static_cast<float_t>((pos.y + offsetY) * TILE_HEIGHT);

            visiblePath.push_back(visibleVertex);
         }

         visiblePath.push_back(visiblePath.at(0));
         mAtmosphere.mOutlines.push_back(visiblePath);
      }

      // create the physical chain
      std::vector<b2Vec2> chain;
      for (auto& pos : path.mScaled)
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
      auto fixture = body->CreateFixture(&fixtureDef);

      // deadly objects are deadly :)
      if (behavior == ObjectBehaviorDeadly)
      {
         auto objectData = new FixtureNode(this);
         objectData->setType(ObjectTypeDeadly);
         fixture->SetUserData(static_cast<void*>(objectData));
      }

      mAtmosphere.mChains.push_back(chain);
   }
}


//-----------------------------------------------------------------------------
void Level::parseAtmosphereLayer(TmxLayer* layer, TmxTileSet* tileSet)
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

   mAtmosphere.mMap.resize(width * height);
   mAtmosphere.mMapWidth = width;
   mAtmosphere.mMapHeight = height;
   mAtmosphere.mMapOffsetX = offsetX;
   mAtmosphere.mMapOffsetY = offsetY;

   for (auto y = 0u; y < height; y++)
   {
      for (auto x = 0u; x < width; x++)
      {
         // get the current tile number
         auto tileNumber = tiles[y * width + x];
         auto tileRelative = static_cast<int32_t>(AtmosphereTileInvalid);
         if (tileNumber != 0)
         {
            tileRelative = tileNumber - tileSet->mFirstGid;
            mAtmosphere.mMap[y * width + x] = tileRelative;
         }

          mAtmosphere.mMap[y * width + x] = tileRelative;
      }
   }
}


//-----------------------------------------------------------------------------
void Level::parsePhysicsTiles(
   TmxLayer* layer,
   TmxTileSet* tileSet,
   const std::filesystem::path& basePath
)
{
   // std::cout << "parsing physics tiles vs. level layer (" << basePath.string() << ")" << std::endl;

   std::ifstream phsyicsFile(basePath / std::filesystem::path("physics_tiles.csv").string());

   std::map<int32_t, std::array<int32_t, 9>> map;
   std::string line;

   while (std::getline(phsyicsFile, line))
   {
      std::istringstream stream(line);
      std::string item;
      std::vector<int32_t> items;

      while (getline(stream, item, ','))
      {
         try
         {
            items.push_back(std::stoi(item));
         }
         catch (const std::invalid_argument& /*e*/)
         {
            // std::cerr << e.what() << std::endl;
         }
         catch (const std::out_of_range& /*e*/)
         {
            // std::cerr << e.what() << std::endl;
         }
      }

      if (items.size() < 10)
      {
         continue;
      }

      std::array<int32_t, 9> data;

      if (items.size() == 11) // a range: 0,12,1,1,1,1,1,1,1,1,1
      {
         std::copy_n(items.begin() + 2, 9, data.begin());
         for (auto key = items[0]; key <= items[1]; key++)
         {
            map[key] = data;
         }
      }
      else if (items.size() == 10) // a single entry: 64,3,3,3,3,0,0,3,0,0
      {
         std::copy_n(items.begin() + 1, 9, data.begin());
         map[items[0]] = data;
      }

      // for (auto x : data)
      // {
      //    std::cout << x << ",";
      // }
      // std::cout << std::endl;
   }

   mPhysics.mGridWidth  = layer->mWidth  * 3;
   mPhysics.mGridHeight = layer->mHeight * 3;
   mPhysics.mGridSize   = mPhysics.mGridWidth * mPhysics.mGridHeight;

   // create a larger grid and copy tile contents in there
   mPhysics.mPhysicsMap.resize(mPhysics.mGridSize);

   auto yi = 0u;
   for (auto y = 0u; y < layer->mHeight; y++)
   {
      for (auto x = 0u; x < layer->mWidth; x++)
      {
         const auto key = layer->mData[y * layer->mWidth + x];

         if (key != 0)
         {
            const auto it = map.find(key - tileSet->mFirstGid);

            // std::cout << key << ",";

            if (it != map.end())
            {
               const auto& arr = (*it).second;

               const auto row1 = ((y + yi + 0) * mPhysics.mGridWidth) + (x * 3);
               const auto row2 = ((y + yi + 1) * mPhysics.mGridWidth) + (x * 3);
               const auto row3 = ((y + yi + 2) * mPhysics.mGridWidth) + (x * 3);

               for (auto xi = 0u; xi < 3; xi++) mPhysics.mPhysicsMap[row1 + xi] = arr[xi + 0];
               for (auto xi = 0u; xi < 3; xi++) mPhysics.mPhysicsMap[row2 + xi] = arr[xi + 3];
               for (auto xi = 0u; xi < 3; xi++) mPhysics.mPhysicsMap[row3 + xi] = arr[xi + 6];
            }
         }
      }

      yi += 2;

      // std::cout << std::endl;
   }

   static const float scale = 0.33333333333333333f;

   SquareMarcher solid(
      mPhysics.mGridWidth,
      mPhysics.mGridHeight,
      mPhysics.mPhysicsMap,
      std::vector<int32_t>{1},
      basePath / std::filesystem::path("physics_path_solid.csv"),
      scale
   );

   solid.writeGridToImage(basePath / std::filesystem::path("physics_grid_solid.png"));
   solid.writePathToImage(basePath / std::filesystem::path("physics_path_solid.png"));

   addPathsToWorld(layer->mOffsetX, layer->mOffsetY, solid.mPaths, ObjectBehaviorSolid);

   SquareMarcher deadly(
      mPhysics.mGridWidth,
      mPhysics.mGridHeight,
      mPhysics.mPhysicsMap,
      std::vector<int32_t>{3},
      basePath / std::filesystem::path("physics_path_deadly.csv"),
      scale
   );

   addPathsToWorld(layer->mOffsetX, layer->mOffsetY, deadly.mPaths, ObjectBehaviorDeadly);
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

   for (auto y = 0u; y < height; y++)
   {
      for (auto x = 0u; x < width; x++)
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
   auto positionCount = poly.size();
   b2Vec2* points = new b2Vec2[positionCount];

   std::vector<p2t::Point*> polyLine;

   for (auto positionIndex = 0u; positionIndex < positionCount; positionIndex++)
   {
      sf::Vector2f pos = poly.at(positionIndex) + sf::Vector2f(offsetX, offsetY);

      points[positionIndex].x = pos.x / PPM;
      points[positionIndex].y = pos.y / PPM;

      // printf(
      //    "pos: %d, x: %f, y: %f\n",
      //    positionIndex,
      //    pos.x / PPM,
      //    pos.y / PPM
      // );

      fflush(stdout);

      p2t::Point* p = new p2t::Point(static_cast<double>(pos.x / PPM), static_cast<double>(pos.y / PPM));
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

   for (auto i = 0u; i < triangles.size(); i++)
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


AtmosphereTile Level::Atmosphere::getTileForPosition(const b2Vec2& pos) const
{
   auto x = pos.x - mMapOffsetX;
   auto y = pos.y - mMapOffsetY;

   if (x < 0 || x >= mMapWidth)
   {
      return AtmosphereTileInvalid;
   }

   if (y < 0 || y >= mMapHeight)
   {
      return AtmosphereTileInvalid;
   }

  //   std::cout
  //     << "physics tile position: "
  //     << playerPos.x << " x " << playerPos.y
  //     << ", translated tile position: "
  //     << x << " x " << y
  //     << std::endl;

   auto tx = static_cast<uint32_t>(x * PPM / TILE_WIDTH);
   auto ty = static_cast<uint32_t>(y * PPM / TILE_HEIGHT);

   AtmosphereTile tile = static_cast<AtmosphereTile>(mMap[ty * mMapWidth + tx]);
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
Bouncer* Level::getNearbyBouncer() const
{
   Bouncer* nearbyBouncer = nullptr;

   for (Bouncer* bouncer : mBouncers)
   {
      if (bouncer->isPlayerAtBouncer())
      {
         nearbyBouncer = bouncer;
         break;
      }
   }

   return nearbyBouncer;
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
void Level::reset()
{
   for (auto& door : mDoors)
   {
      door->reset();
   }
}


//-----------------------------------------------------------------------------
std::map<b2Body*, size_t>* Level::getPointSizeMap()
{
   return &mPointCountMap;
}


//-----------------------------------------------------------------------------
std::map<b2Body *, b2Vec2 *>* Level::getPointMap()
{
   return &mPointMap;
}


