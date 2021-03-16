#include "level.h"

// game
#include "camerapane.h"
#include "checkpoint.h"
#include "constants.h"
#include "debugdraw.h"
#include "dialogue.h"
#include "displaymode.h"
#include "extraitem.h"
#include "extramanager.h"
#include "fixturenode.h"
#include "framework/math/maptools.h"
#include "framework/math/sfmlmath.h"
#include "framework/tools/checksum.h"
#include "framework/tools/globalclock.h"
#include "gameconfiguration.h"
#include "gamecontactlistener.h"
#include "leveldescription.h"
#include "levelmap.h"
#include "luainterface.h"
#include "mechanisms/bouncer.h"
#include "mechanisms/conveyorbelt.h"
#include "mechanisms/crusher.h"
#include "mechanisms/deathblock.h"
#include "mechanisms/door.h"
#include "mechanisms/fan.h"
#include "mechanisms/laser.h"
#include "mechanisms/lever.h"
#include "mechanisms/moveablebox.h"
#include "mechanisms/movingplatform.h"
#include "mechanisms/spikeball.h"
#include "mechanisms/spikes.h"
#include "meshtools.h"
#include "physics/physicsconfiguration.h"
#include "player/player.h"
#include "savestate.h"
#include "squaremarcher.h"
#include "tilemap.h"
#include "weather.h"

// sfml
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include "framework/tmxparser/tmxelement.h"
#include "framework/tmxparser/tmximage.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmximagelayer.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxobjectgroup.h"
#include "framework/tmxparser/tmxpolygon.h"
#include "framework/tmxparser/tmxpolyline.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxparser.h"
#include "framework/tmxparser/tmxtileset.h"
#include "framework/tmxparser/tmxtools.h"

// poly2tri
#include "poly2tri/poly2tri.h"
#include "poly2tri/common/shapes.h"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

// hardcoded settings
// #define GLOW_ENABLED


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
const Atmosphere& Level::getPhysics() const
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

   mLevelBackgroundRenderTexture.reset();

   mAtmosphereShader.reset();
   mGammaShader.reset();
   mBlurShader.reset();
   mDeathShader.reset();

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

   mAtmosphereShader = std::make_unique<AtmosphereShader>(textureWidth, textureHeight);
   mGammaShader = std::make_unique<GammaShader>();
   mBlurShader = std::make_unique<BlurShader>(textureWidth, textureHeight);
   mDeathShader = std::make_unique<DeathShader>(textureWidth, textureHeight);

   // keep track of those textures
   mRenderTextures.clear();
   mRenderTextures.push_back(mLevelRenderTexture);
   mRenderTextures.push_back(mLevelBackgroundRenderTexture);
   for (const auto& fb : mRenderTextures)
   {
      std::cout << "[x] created render texture: " << fb->getSize().x << " x " << fb->getSize().y << std::endl;
   }

   mAtmosphereShader->initialize();
   mGammaShader->initialize(mLevelRenderTexture->getTexture());
   mBlurShader->initialize();
   mDeathShader->initialize();
}


//-----------------------------------------------------------------------------
Level::Level()
  : GameNode(nullptr)
{
   setName(typeid(Level).name());

   // init world for this level
   b2Vec2 gravity(0.f, PhysicsConfiguration::getInstance().mGravity);

   LuaInterface::instance()->reset();

   // clear those here so the world destructor doesn't double-delete them
   Projectile::clear();

   mWorld = std::make_shared<b2World>(gravity);

   GameContactListener::getInstance()->reset();
   mWorld->SetContactListener(GameContactListener::getInstance());

   sCurrentLevel = this;

   mRaycastLight = std::make_shared<RaycastLight>();
   mStaticLight = std::make_shared<StaticLight>();

   // add raycast light for player
   mPlayerLight = RaycastLight::deserialize(nullptr);
   mPlayerLight->mSprite.setColor(sf::Color(255, 255, 255, 10));
   mRaycastLight->mLights.push_back(mPlayerLight);

   mMap = std::make_unique<LevelMap>();

   mMechanisms = {
      &mBouncers,
      &mConveyorBelts,
      &mCrushers,
      &mDoors,
      &mFans,
      &mLasers,
      &mLevers,
      &mPlatforms,
      &mPortals,
      &mSpikeBalls,
      &mSpikes,
      &mMoveableBoxes,
      &mDeathBlocks
   };
}


//-----------------------------------------------------------------------------
Level::~Level()
{
   std::cout << "[x] deleting current level" << std::endl;

   // properly delete point map
   for (auto& kv : mPointMap)
   {
      delete kv.second;
   }
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
         parallax = itParallaxValue->second->mValueFloat.value();
      }

      auto itParallaxView = map.find("parallax_view");
      if (itParallaxView != map.end())
      {
         const auto view = itParallaxView->second->mValueInt.value();
         mParallaxFactors[view] = parallax;
      }
   }
}


//-----------------------------------------------------------------------------
void Level::loadTmx()
{
   auto path = std::filesystem::path(mDescription->mFilename).parent_path();

   const auto checksumOld = Checksum::readChecksum(mDescription->mFilename + ".crc");
   const auto checksumNew = Checksum::calcChecksum(mDescription->mFilename);
   if (checksumOld != checksumNew)
   {
      std::cout << "[x] checksum mismatch, deleting cached data" << std::endl;
      std::filesystem::remove(path / "physics_grid_solid.png");
      std::filesystem::remove(path / "physics_path_deadly.csv");
      std::filesystem::remove(path / "physics_path_solid.csv");
      std::filesystem::remove(path / "physics_path_solid.png");
      std::filesystem::remove(path / "layer_level_solid_not_optimised.obj");
      Checksum::writeChecksum(mDescription->mFilename + ".crc", checksumNew);
   }

   sf::Clock elapsed;

   // parse tmx
   std::cout << "[x] parsing tmx: " << mDescription->mFilename << std::endl;

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

         if (layer->mName.rfind("doors", 0) == 0)
         {
            mDoors = Door::load(layer, tileset, path, mWorld);
         }
         else if (layer->mName == "fans")
         {
            Fan::load(layer, tileset, mWorld);
         }
         else if (layer->mName == "lasers")
         {
            const auto lasers = Laser::load(layer, tileset, path, mWorld);
            mLasers.insert(mLasers.end(), lasers.begin(), lasers.end());
         }
         else if (layer->mName == "lasers_2") // support for dstar's new laser tileset
         {
            const auto lasers = Laser::load(layer, tileset, path, mWorld);
            mLasers.insert(mLasers.end(), lasers.begin(), lasers.end());
         }
         else if (layer->mName == "levers")
         {
            mLevers = Lever::load(layer, tileset, path, mWorld);
         }
         else if (layer->mName == "platforms")
         {
            mPlatforms = MovingPlatform::load(layer, tileset, path, mWorld);
         }
         else if (layer->mName == "portals")
         {
            mPortals = Portal::load(layer, tileset, path, mWorld);
         }
         else if (layer->mName == "toggle_spikes")
         {
            auto spikes = Spikes::load(layer, tileset, path, Spikes::Mode::Toggled);
            for (const auto &s : spikes)
            {
               mSpikes.push_back(s);
            }
         }
         else if (layer->mName == "trap_spikes")
         {
            auto spikes = Spikes::load(layer, tileset, path, Spikes::Mode::Trap);
            for (const auto &s : spikes)
            {
               mSpikes.push_back(s);
            }
         }
         else if (layer->mName == "interval_spikes")
         {
            auto spikes = Spikes::load(layer, tileset, path, Spikes::Mode::Interval);
            for (const auto &s : spikes)
            {
               mSpikes.push_back(s);
            }
         }
         else // tile map
         {
            std::shared_ptr<TileMap> tileMap = std::make_shared<TileMap>();
            tileMap->load(layer, tileset, path);

            auto pushTileMap = true;

            if (layer->mName == "atmosphere")
            {
               mAtmosphere.mTileMap = tileMap;
               mAtmosphere.parse(layer, tileset);
            }
            else if (layer->mName == "extras")
            {
               Player::getCurrent()->getExtraManager()->mTilemap = tileMap;
               Player::getCurrent()->getExtraManager()->load(layer, tileset);
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

            if (pushTileMap)
            {
               mTileMaps.push_back(tileMap);
            }
         }
      }

      else if (element->mType == TmxElement::TypeObjectGroup)
      {
         TmxObjectGroup* objectGroup = dynamic_cast<TmxObjectGroup*>(element);

         for (const auto& object : objectGroup->mObjects)
         {
            TmxObject* tmxObject = object.second;

            if (objectGroup->mName == "lasers" || objectGroup->mName == "lasers_2")
            {
               Laser::addObject(tmxObject);
            }
            else if (objectGroup->mName == "enemies")
            {
               Enemy enemy;
               enemy.parse(tmxObject);
               mEnemyDataFromTmxLayer[enemy.mId]=enemy;
            }
            else if (objectGroup->mName == "fans")
            {
               Fan::addObject(tmxObject, path);
            }
            else if (objectGroup->mName == "portals")
            {
               if (tmxObject->mPolyLine)
               {
                  Portal::link(mPortals, tmxObject);
               }
            }
            else if (objectGroup->mName == "smoke")
            {
               auto smoke = SmokeEffect::deserialize(tmxObject, objectGroup);
               mSmokeEffect.push_back(smoke);
            }
            else if (objectGroup->mName == "spike_balls")
            {
               auto spikeBall = std::make_shared<SpikeBall>(dynamic_cast<GameNode*>(this));
               spikeBall->setup(tmxObject, mWorld);
               mSpikeBalls.push_back(spikeBall);
            }
            else if (objectGroup->mName == "moveable_objects")
            {
               auto box = std::make_shared<MoveableBox>(dynamic_cast<GameNode*>(this));
               box->setup(tmxObject, mWorld);
               mMoveableBoxes.push_back(box);
            }
            else if (objectGroup->mName == "death_blocks")
            {
               auto deathBlock = std::make_shared<DeathBlock>(dynamic_cast<GameNode*>(this));
               deathBlock->setup(tmxObject, mWorld);
               mDeathBlocks.push_back(deathBlock);
            }
            else if (objectGroup->mName == "checkpoints")
            {
               const auto cpi = Checkpoint::add(tmxObject);
               auto cp = Checkpoint::getCheckpoint(cpi);

               // whenever we reach a checkpoint, update the checkpoint index in the save state
               cp->addCallback([cpi](){SaveState::getCurrent().mCheckpoint = cpi;});

               // whenever we reach a checkpoint, serialize the save state
               cp->addCallback([](){SaveState::serializeToFile();});
            }
            else if (objectGroup->mName == "dialogues")
            {
               Dialogue::add(tmxObject);
            }
            else if (objectGroup->mName == "bouncers")
            {
               auto bouncer = std::make_shared<Bouncer>(
                  dynamic_cast<GameNode*>(this),
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
               auto belt = std::make_shared<ConveyorBelt>(
                  dynamic_cast<GameNode*>(this),
                  mWorld,
                  tmxObject,
                  path
               );

               mConveyorBelts.push_back(belt);

               addDebugRect(
                  belt->getBody(),
                  tmxObject->mX,
                  tmxObject->mY,
                  tmxObject->mWidth,
                  tmxObject->mHeight
               );
            }
            else if (objectGroup->mName == "crushers")
            {
               auto crusher = std::make_shared<Crusher>(dynamic_cast<GameNode*>(this));
               crusher->setup(tmxObject, mWorld);
               mCrushers.push_back(crusher);
            }
            else if (objectGroup->mName == "rooms")
            {
               Room::deserialize(tmxObject, mRooms);
            }
            else if (objectGroup->mName == "platform_paths")
            {
               if (tmxObject->mPolyLine)
               {
                  MovingPlatform::link(mPlatforms, tmxObject);
               }
            }
            else if (objectGroup->mName == "weather")
            {
               sf::IntRect rect{
                  static_cast<int32_t>(tmxObject->mX),
                  static_cast<int32_t>(tmxObject->mY),
                  static_cast<int32_t>(tmxObject->mWidth),
                  static_cast<int32_t>(tmxObject->mHeight)
               };

               if (tmxObject->mName.rfind("rain", 0) == 0)
               {
                  Weather::getInstance().add(Weather::WeatherType::Rain, rect);
               }
            }
            else if (objectGroup->mName == "lights")
            {
               auto light = RaycastLight::deserialize(tmxObject);
               mRaycastLight->mLights.push_back(light);
            }
            else if (objectGroup->mName.compare(0, StaticLight::sLayerName.size(), StaticLight::sLayerName) == 0)
            {
               auto light = StaticLight::deserialize(tmxObject, objectGroup);
               mStaticLight->mLights.push_back(light);
            }
            if (objectGroup->mName == "switchable_objects")
            {
               Lever::addSearchRect(tmxObject);
            }
         }
      }

      else if (element->mType == TmxElement::TypeImageLayer)
      {
         auto image = ImageLayer::deserialize(element, path);
         mImageLayers.push_back(image);
      }
   }

   Laser::merge();
   Fan::merge();
   mFans = Fan::getFans();
   Lever::merge(mLevers, mLasers, mPlatforms, mFans, mConveyorBelts, mSpikes);

   mMap->loadLevelTextures(
      path / std::filesystem::path("physics_grid_solid.png"),
      path / std::filesystem::path("physics_path_solid.png")
   );

   mMap->setDoors(mDoors);
   mMap->setPortals(mPortals);

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

   Weather::getInstance().clear();
   Checkpoint::resetAll();
   Dialogue::resetAll();

   // load tmx
   loadTmx();

   // load static lights
   std::cout << "[x] loading static lights..." << std::endl;
   if (!mStaticLight->mLights.empty())
   {
      mStaticLight->load();
   }

   // load raycast lights
   std::cout << "[x] loading raycast lights..." << std::endl;
   if (!mRaycastLight->mLights.empty())
   {
      mRaycastLight->load();
   }

   // loading ao
   std::cout << "[x] loading ao... " << std::endl;
   mAo.load(path, std::filesystem::path(mDescription->mFilename).stem().string());

   std::cout << "[x] level loading complete" << std::endl;
}


//-----------------------------------------------------------------------------
void Level::initialize()
{
   createViews();

   mDescription = LevelDescription::load(mDescriptionFilename);

   load();

   mStartPosition.x = static_cast<float_t>(mDescription->mStartPosition.at(0) * PIXELS_PER_TILE  + PLAYER_ACTUAL_WIDTH / 2);
   mStartPosition.y = static_cast<float_t>(mDescription->mStartPosition.at(1) * PIXELS_PER_TILE + DIFF_PLAYER_TILE_TO_PHYSICS);

   loadCheckpoint();

   spawnEnemies();
}


//-----------------------------------------------------------------------------
void Level::loadCheckpoint()
{
   auto checkpointIndex = SaveState::getCurrent().mCheckpoint;
   auto checkpoint = Checkpoint::getCheckpoint(checkpointIndex);

   if (checkpoint)
   {
      auto pos = checkpoint->calcCenter();
      mStartPosition.x = static_cast<float>(pos.x);
      mStartPosition.y = static_cast<float>(pos.y);
      std::cout << "[-] move to checkpoint: " << checkpointIndex << std::endl;
   }
   else
   {
      std::cerr << "[!] level doesn't have a start check point set up, falling back to start position" << std::endl;
   }
}


//-----------------------------------------------------------------------------
void Level::reset()
{
   for (auto& door : mDoors)
   {
      door.reset();
   }
}


//-----------------------------------------------------------------------------
void Level::resetDeathShader()
{
   mDeathShader->reset();
}


//-----------------------------------------------------------------------------
void Level::spawnEnemies()
{
   // deprecated approach:
   // merge enemy layer from tmx with enemy info that's stored inside json
   // iterate through all enemies in the json
   for (auto& jsonDescription : mDescription->mEnemies)
   {
      auto luaNode = LuaInterface::instance()->addObject(std::string("data/scripts/enemies/") + jsonDescription.mScript);

      // find matching enemy data from the tmx layer and retrieve the patrol path from there
      const auto& it = mEnemyDataFromTmxLayer.find(jsonDescription.mId);
      if (it != mEnemyDataFromTmxLayer.end())
      {
         // positions from the tmx are given in pixels, not tiles
         jsonDescription.mPositionGivenInTiles = false;

         jsonDescription.mStartPosition.push_back(it->second.mPixelPosition.x);
         jsonDescription.mStartPosition.push_back(it->second.mPixelPosition.y);

         if (jsonDescription.mGeneratePatrolPath)
         {
            it->second.addPaths(mWorldChains);
         }

         if (!it->second.mPixelPath.empty())
         {
            jsonDescription.mPath = it->second.mPixelPath;
         }

         // merge properties from tmx with those loaded from json
         for (auto& property : it->second.mProperties)
         {
            jsonDescription.mProperties.push_back(property);
         }
      }

      // initialize lua node and store enemy
      luaNode->mEnemyDescription = jsonDescription;
      luaNode->initialize();
      mEnemies.push_back(luaNode);
   }

   // those enemies that have a lua script associated inside the tmx layer don't need
   // additional information from json, those can just be spawned.
   // this should probably be the future and only approach how to handle enemy spawning.
   for (auto& it : mEnemyDataFromTmxLayer)
   {
      auto script = it.second.findProperty("script");

      if (script.has_value())
      {
         auto luaNode = LuaInterface::instance()->addObject(std::string("data/scripts/enemies/") + script.value().mValue);

         EnemyDescription jsonDescription;
         jsonDescription.mPositionGivenInTiles = false;
         jsonDescription.mStartPosition.push_back(it.second.mPixelPosition.x);
         jsonDescription.mStartPosition.push_back(it.second.mPixelPosition.y);

         if (jsonDescription.mGeneratePatrolPath)
         {
            it.second.addPaths(mWorldChains);
         }

         if (!it.second.mPixelPath.empty())
         {
            jsonDescription.mPath = it.second.mPixelPath;
         }

         // merge properties from tmx with those loaded from json
         for (auto& property : it.second.mProperties)
         {
            jsonDescription.mProperties.push_back(property);
         }

         // initialize lua node and store enemy
         luaNode->mEnemyDescription = jsonDescription;
         luaNode->initialize();
         mEnemies.push_back(luaNode);
      }
   }
}


//-----------------------------------------------------------------------------
void Level::drawStaticChains(sf::RenderTarget& target)
{
   for (auto& path : mAtmosphere.mOutlines)
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
   auto& gameConfig = GameConfiguration::getInstance();

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

   // update room
   mCurrentRoom = Room::find(Player::getCurrent()->getPixelPositionf(), mRooms);
   const auto prevRoomId = mCurrentRoomId;
   mCurrentRoomId = mCurrentRoom.has_value() ? mCurrentRoom->mId : -1;

   if (prevRoomId != mCurrentRoomId)
   {
      std::cout
         << "[i] player moved to room: "
         << (mCurrentRoom.has_value() ? mCurrentRoom->mName : "undefined")
         << std::endl;

      cameraSystem.setRoom(mCurrentRoom);
   }

   // update camera system
   cameraSystem.update(mViewWidth, mViewHeight);

   const auto lookVector = CameraPane::getInstance().getLookVector();
   auto levelViewX = cameraSystem.getX() + lookVector.x;
   auto levelViewY = cameraSystem.getY() + lookVector.y;

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
void Level::drawPlayer(sf::RenderTarget& target)
{
   auto player = Player::getCurrent();

   if (player->isDead())
   {
      auto deathRenderTexture = mDeathShader->getRenderTexture();

      // render player to texture
      deathRenderTexture->clear(sf::Color{0, 0, 0, 0});
      deathRenderTexture->setView(*mLevelView);
      player->draw(*deathRenderTexture);
      deathRenderTexture->display();

      // render texture with shader applied
      auto deathShaderSprite = sf::Sprite(deathRenderTexture->getTexture());

      // TODO: have a static view for rendertexture quads
      sf::View view(
         sf::FloatRect(
            0.0f,
            0.0f,
            static_cast<float>(mLevelRenderTexture->getSize().x),
            static_cast<float>(mLevelRenderTexture->getSize().y)
         )
      );

      view.setViewport(sf::FloatRect(0.0f, 0.0f, 1.0f, 1.0f));
      target.setView(view);
      target.draw(deathShaderSprite, &mDeathShader->getShader());

      takeScreenshot("screenshot_death_anim", *mDeathShader->getRenderTexture());

      target.setView(*mLevelView);
   }
   else
   {
      player->draw(target);
   }
}


void Level::drawLayers(sf::RenderTarget& target, int32_t from, int32_t to)
{
   target.setView(*mLevelView);

   for (auto z = from; z <= to; z++)
   {
      mStaticLight->drawToZ(target, {}, z);

      for (const auto& smoke : mSmokeEffect)
      {
         smoke->drawToZ(target, {}, z);
      }

      // TODO: it's not expected that tiles are in different z layers
      //       and then unify them in one big loop

      for (auto& tileMap : mTileMaps)
      {
         if (tileMap->getZ() == z)
         {
            target.draw(*tileMap);
         }
      }

      for (auto& mechanismVector : mMechanisms)
      {
         for (auto& mechanism : *mechanismVector)
         {
            if (mechanism->getZ() == z)
            {
               mechanism->draw(target);
            }
         }
      }

      // enemies
      for (auto& enemy : mEnemies)
      {
         if (enemy->mZ == z)
         {
            enemy->draw(target);
         }
      }

      if (z == ZDepthPlayer)
      {
         // ambient occlusion
         mAo.draw(target);

         // draw player
         drawPlayer(target);
      }

      for (auto& layer : mImageLayers)
      {
         if (layer->mZ == z)
         {
            target.draw(layer->mSprite, {layer->mBlendMode});
         }
      }
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

   // draw elements that are supposed to glow / to be blurred here

#ifdef GLOW_ENABLED
   // lasers have been removed here because dstar added the glow to the spriteset

   const auto pPos = Player::getCurrent()->getPixelPositionf();

   // draw lasers
   for (auto laser : mLasers)
   {
      const auto lPos = std::dynamic_pointer_cast<Laser>(laser)->getPixelPosition();
      if (SfmlMath::lengthSquared(lPos - pPos) > 250000)
      {
         continue;
      }

      laser->draw(target);
   }
#endif
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
void Level::takeScreenshot(const std::string& basename, sf::RenderTexture& texture)
{
   if (!mScreenshot)
   {
      return;
   }

   std::ostringstream ss;
   ss << basename << "_" << std::setw(2) << std::setfill('0') << mScreenshotCounters[basename] << ".png";
   mScreenshotCounters[basename]++;
   texture.getTexture().copyToImage().saveToFile(ss.str());
}


//-----------------------------------------------------------------------------
// Level Rendering Flow
//
//    textures/render targets:
//    - atmosphere texture
//    - level background texture
//    - level texture
//    - window
//
//    01) draw atmosphere (air / water)                           -> atmosphere texture
//    02) draw parallax info                                      -> level background texture
//    03) draw level background with atmosphere shader enabled    -> background texture
//        - layers z=0..15
//    04) draw level background                                   -> level texture
//    05) draw level foreground                                   -> level texture
//        - layers z=16..50
//        - additive lights
//        - smoke (z=20)
//        - mechanisms
//        - ambient occlusion
//        - images with varying blend modes
//        - player
//    06) draw raycast lights                                     -> level texture
//    07) draw projectiles                                        -> level texture
//    08) flash and bounce -> move level texture
//    09) draw level texture with gamma shader enabled            -> straight to window
//    10) draw level map (if enabled)                             -> straight to window
//
void Level::draw(
   const std::shared_ptr<sf::RenderTexture>& window,
   bool screenshot
)
{
   mScreenshot = screenshot;

   // render atmosphere to atmosphere texture, that texture is used in the shader only
   mAtmosphereShader->getRenderTexture()->clear();
   drawAtmosphereLayer(*mAtmosphereShader->getRenderTexture().get());
   mAtmosphereShader->getRenderTexture()->display();
   takeScreenshot("screenshot_atmosphere", *mAtmosphereShader->getRenderTexture().get());

   // render glowing elements
#ifdef GLOW_ENABLED
   mBlurShader->clearTexture();
   drawBlurLayer(*mBlurShader->getRenderTexture().get());
   mBlurShader->getRenderTexture()->display();
   takeScreenshot("screenshot_blur", *mBlurShader->getRenderTexture().get());
#endif

   // render layers affected by the atmosphere
   mLevelBackgroundRenderTexture->clear();

   updateViews();
   drawParallaxMaps(*mLevelBackgroundRenderTexture.get());
   drawLayers(*mLevelBackgroundRenderTexture.get(), ZDepthBackgroundMin, ZDepthBackgroundMax);
   mLevelBackgroundRenderTexture->display();
   takeScreenshot("screenshot_level_background", *mLevelBackgroundRenderTexture.get());

   // draw the atmospheric parts into the level texture
   sf::Sprite backgroundSprite(mLevelBackgroundRenderTexture->getTexture());
   mAtmosphereShader->update();
   mLevelRenderTexture->draw(backgroundSprite, &mAtmosphereShader->getShader());

#ifdef GLOW_ENABLED
   sf::Sprite blurSprite(mBlurShader->getRenderTexture()->getTexture());
   const auto downScaleX = mBlurShader->getRenderTextureScaled()->getSize().x / static_cast<float>(mBlurShader->getRenderTexture()->getSize().x);
   const auto downScaleY = mBlurShader->getRenderTextureScaled()->getSize().y / static_cast<float>(mBlurShader->getRenderTexture()->getSize().y);
   blurSprite.scale({downScaleX, downScaleY});

   sf::RenderStates statesShader;
   mBlurShader->update();
   statesShader.shader = &mBlurShader->getShader();
   mBlurShader->getRenderTextureScaled()->draw(blurSprite, statesShader);

   sf::Sprite blurScaleSprite(mBlurShader->getRenderTextureScaled()->getTexture());
   blurScaleSprite.scale(1.0f / downScaleX, 1.0f / downScaleY);
   blurScaleSprite.setTextureRect(
      sf::IntRect(
         0,
         static_cast<int32_t>(blurScaleSprite.getTexture()->getSize().y),
         static_cast<int32_t>(blurScaleSprite.getTexture()->getSize().x),
         -static_cast<int32_t>(blurScaleSprite.getTexture()->getSize().y)
      )
   );

   sf::RenderStates statesAdd;
   statesAdd.blendMode = sf::BlendAdd;
   mLevelRenderTexture->draw(blurScaleSprite, statesAdd);
#endif

   // draw the level layers into the level texture
   drawLayers(*mLevelRenderTexture.get(), ZDepthForegroundMin, ZDepthForegroundMax);

   // draw all the other things
   drawRaycastLight(*mLevelRenderTexture.get());
   Weapon::drawProjectileHitAnimations(*mLevelRenderTexture.get());

   if (DisplayMode::getInstance().isSet(Display::DisplayDebug))
   {
      drawStaticChains(*mLevelRenderTexture.get());
      DebugDraw::debugBodies(*mLevelRenderTexture.get(), this);
      DebugDraw::drawRect(*mLevelRenderTexture.get(), Player::getCurrent()->getPlayerPixelRect());

      for (const auto& room : mRooms)
      {
         for (const auto& rect : room.mRects)
         {
            DebugDraw::drawRect(*mLevelRenderTexture.get(), rect, sf::Color::Yellow);
         }
      }
   }

   // display the whole texture
   sf::View view(
      sf::FloatRect(
         0.0f,
         0.0f,
         static_cast<float>(mLevelRenderTexture->getSize().x),
         static_cast<float>(mLevelRenderTexture->getSize().y)
      )
   );

   view.setViewport(sf::FloatRect(0.0f, 0.0f, 1.0f, 1.0f));
   mLevelRenderTexture->setView(view);
   mLevelRenderTexture->display();
   takeScreenshot("screenshot_level", *mLevelRenderTexture.get());

   auto levelTextureSprite = sf::Sprite(mLevelRenderTexture->getTexture());

   levelTextureSprite.setPosition(mBoomEffect.mBoomOffsetX, mBoomEffect.mBoomOffsetY);
   levelTextureSprite.scale(mViewToTextureScale, mViewToTextureScale);

   mGammaShader->update();
   window->draw(levelTextureSprite, &mGammaShader->getGammaShader());

   if (DisplayMode::getInstance().isSet(Display::DisplayMap))
   {
      mMap->draw(*window.get());
   }
}


//-----------------------------------------------------------------------------
void Level::updatePlayerLight()
{
   mPlayerLight->mPosMeters = Player::getCurrent()->getBody()->GetPosition();
   mPlayerLight->updateSpritePosition();

   // the player, once he dies, becomes inactive and just sinks down
   // so the player light is disabled to avoid any glitches
   mPlayerLight->mSprite.setColor(sf::Color(255, 255, 255, Player::getCurrent()->isDead()? 0 : 10));
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

   Checkpoint::update();
   Dialogue::update();

   for (auto& tileMap : mTileMaps)
   {
      tileMap->update(dt);
   }

   for (auto mechanismVector : mMechanisms)
   {
      for (auto& mechanism : *mechanismVector)
      {
         mechanism->update(dt);
      }
   }

   LuaInterface::instance()->update(dt);

   updatePlayerLight();

   mStaticLight->update(GlobalClock::getInstance()->getElapsedTime(), 0.0f, 0.0f);
   mRaycastLight->update(GlobalClock::getInstance()->getElapsedTime(), 0.0f, 0.0f);

   for (const auto& smoke : mSmokeEffect)
   {
      smoke->update(GlobalClock::getInstance()->getElapsedTime(), 0.0f, 0.0f);
   }

   mDeathShader->update(dt);
}


//-----------------------------------------------------------------------------
const std::shared_ptr<b2World>& Level::getWorld() const
{
   return mWorld;
}


//-----------------------------------------------------------------------------
void Level::addChainToWorld(
   const std::vector<b2Vec2>& chain,
   ObjectBehavior behavior
)
{
   // it's easier to store all the physics chains in a separate data structure
   // than to parse the box2d world every time we want those loops.
   mWorldChains.push_back(chain);

   b2ChainShape chainShape;
   chainShape.CreateLoop(&chain.at(0), static_cast<int32_t>(chain.size()));

   b2FixtureDef fixtureDef;
   fixtureDef.density = 0.0f;
   fixtureDef.friction = 0.2f;
   fixtureDef.shape = &chainShape;

   b2BodyDef bodyDef;
   bodyDef.position.Set(0, 0);
   bodyDef.type = b2_staticBody;

   b2Body* body = mWorld->CreateBody(&bodyDef);

   auto fixture = body->CreateFixture(&fixtureDef);

   auto objectData = new FixtureNode(this);

   // deadly objects are deadly :)
   if (behavior == ObjectBehaviorDeadly)
   {
      objectData->setType(ObjectTypeDeadly);
   }
   else if (behavior == ObjectBehaviorSolid)
   {
      objectData->setType(ObjectTypeWall);
   }

   fixture->SetUserData(static_cast<void*>(objectData));
}


//-----------------------------------------------------------------------------
void Level::addDebugOutlines(
   int32_t offsetX,
   int32_t offsetY,
   std::vector<sf::Vector2f> positions,
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

   // path.printPoly();
   std::vector<sf::Vertex> visiblePath;
   for (auto& pos : positions)
   {
      sf::Vertex visibleVertex;
      visibleVertex.color = color;
      visibleVertex.position.x = static_cast<float_t>((pos.x + offsetX) * PIXELS_PER_TILE);
      visibleVertex.position.y = static_cast<float_t>((pos.y + offsetY) * PIXELS_PER_TILE);

      visiblePath.push_back(visibleVertex);
   }

   visiblePath.push_back(visiblePath.at(0));
   mAtmosphere.mOutlines.push_back(visiblePath);
}


//-----------------------------------------------------------------------------
void Level::addPathsToWorld(
   int32_t offsetX,
   int32_t offsetY,
   const std::vector<SquareMarcher::Path>& paths,
   ObjectBehavior behavior
)
{
   // just for debugging purposes, this section can be removed later
   for (auto& path : paths)
   {
      const auto& scaled = path.mScaled;
      addDebugOutlines(offsetX, offsetY, scaled, behavior);
   }

   // create the physical chain with 1 body per chain
   for (auto& path : paths)
   {
      std::vector<b2Vec2> chain;
      for (auto& pos : path.mScaled)
      {
         chain.push_back({
               (pos.x + offsetX) * PIXELS_PER_TILE / PPM,
               (pos.y + offsetY) * PIXELS_PER_TILE / PPM
            }
         );
      }

      addChainToWorld(chain, behavior);
   }
}


//-----------------------------------------------------------------------------
void Level::parseObj(
   TmxLayer* layer,
   ObjectBehavior behavior,
   const std::filesystem::path& path
)
{
   std::vector<b2Vec2> points;
   std::vector<std::vector<uint32_t>> faces;
   Mesh::readObj(path.string(), points, faces);
   for (const auto& face : faces)
   {
      std::vector<b2Vec2> chain;
      std::vector<sf::Vector2f> debugPath;
      for (auto index : face)
      {
         const auto& p = points[index];
         chain.push_back({
               (p.x + layer->mOffsetX) / PPM,
               (p.y + layer->mOffsetY) / PPM
            }
         );

         debugPath.push_back({
               p.x / PIXELS_PER_TILE,
               p.y / PIXELS_PER_TILE
            }
         );
      }

      // creating a box2d chain is automatically closing the path
      chain.pop_back();

      addChainToWorld(chain, behavior);
      addDebugOutlines(layer->mOffsetX, layer->mOffsetY, debugPath, behavior);

      // Mesh::writeVerticesToImage(points, faces, {1200, 1200}, "yo_yo.png");
   }
}


//-----------------------------------------------------------------------------
void Level::parsePhysicsTiles(
   TmxLayer* layer,
   TmxTileSet* tileSet,
   const std::filesystem::path& basePath
)
{
   static const float scale = 1.0f / 3.0f;

   auto pathSolidOptimized = basePath / std::filesystem::path("layer_" + layer->mName + "_solid.obj");

   std::cout << "[x] loading: " << pathSolidOptimized.make_preferred().generic_string() << std::endl;

   mPhysics.parse(layer, tileSet, basePath);

   // this whole block should be generated by an external tool
   // right now the squaremarcher output is still used for the in-game map visualization
   SquareMarcher solid(
      mPhysics.mGridWidth,
      mPhysics.mGridHeight,
      mPhysics.mPhysicsMap,
      std::vector<int32_t>{1},
      basePath / std::filesystem::path("physics_path_solid.csv"),
      scale
   );

   solid.writeGridToImage(basePath / std::filesystem::path("physics_grid_solid.png")); // not needed
   solid.writePathToImage(basePath / std::filesystem::path("physics_path_solid.png")); // needed from obj as well

   if (std::filesystem::exists(pathSolidOptimized))
   {
      parseObj(layer, ObjectBehavior::ObjectBehaviorSolid, pathSolidOptimized);
   }
   else
   {
      const auto pathSolidNotOptimised = basePath / std::filesystem::path("layer_" + layer->mName + "_solid_not_optimised.obj");

      // dump the tileset into an obj file, optimise that and load it
      if (mPhysics.dumpObj(layer, tileSet, pathSolidNotOptimised))
      {
#ifdef __linux__
          auto cmd = std::string("tools/path_merge/path_merge") + " "
                + pathSolidNotOptimised.string() + " "
                + pathSolidOptimized.string();
#else
          auto cmd = std::string("tools\\path_merge\\path_merge.exe") + " "
                + pathSolidNotOptimised.string() + " "
                + pathSolidOptimized.string();
#endif

         std::cout << "[x] running cmd: " << cmd << std::endl;

         if (std::system(cmd.c_str()) != 0)
         {
            std::cerr << "[!] command failed" << std::endl;
         }
         else
         {
            std::cout << "[x] command succeeded" << std::endl;
         }
      }
      else
      {
         std::cerr << "[!] dumping unoptimized obj (" << pathSolidNotOptimised<< ") failed" << std::endl;
      }

      // fallback to square marched level
      if (!std::filesystem::exists(pathSolidOptimized))
      {
         std::cerr << "[!] could not find " << pathSolidOptimized.string() << ", obj generator failed" << std::endl;
         addPathsToWorld(layer->mOffsetX, layer->mOffsetY, solid.mPaths, ObjectBehaviorSolid);
      }
      else
      {
         // parse the optimised obj
         parseObj(layer, ObjectBehavior::ObjectBehaviorSolid, pathSolidOptimized);
      }
   }

   // layer of deadly objects
   const auto pathDeadly = basePath / std::filesystem::path("layer_" + layer->mName + "_deadly.obj");
   if (std::filesystem::exists(pathDeadly))
   {
      parseObj(layer, ObjectBehavior::ObjectBehaviorDeadly, pathDeadly);
   }
   else
   {
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
}


//-----------------------------------------------------------------------------
const sf::Vector2f &Level::getStartPosition() const
{
   return mStartPosition;
}


//-----------------------------------------------------------------------------
Level* Level::getCurrentLevel()
{
   return sCurrentLevel;
}


//-----------------------------------------------------------------------------
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
AtmosphereTile Atmosphere::getTileForPosition(const b2Vec2& pos) const
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

   auto tx = static_cast<uint32_t>(x * PPM / PIXELS_PER_TILE);
   auto ty = static_cast<uint32_t>(y * PPM / PIXELS_PER_TILE);

   AtmosphereTile tile = static_cast<AtmosphereTile>(mMap[ty * mMapWidth + tx]);
   return tile;
}


//-----------------------------------------------------------------------------
std::shared_ptr<Portal> Level::getNearbyPortal()
{
   std::shared_ptr<Portal> nearbyPortal;

   for (auto& p : mPortals)
   {
      auto portal = std::dynamic_pointer_cast<Portal>(p);
      if (portal->isPlayerAtPortal())
      {
         nearbyPortal = portal;
         break;
      }
   }

   return nearbyPortal;
}


//-----------------------------------------------------------------------------
std::shared_ptr<Bouncer> Level::getNearbyBouncer()
{
   std::shared_ptr<Bouncer> nearbyBouncer;

   for (auto tmp : mBouncers)
   {
      auto bouncer = std::dynamic_pointer_cast<Bouncer>(tmp);
      if (bouncer->isPlayerAtBouncer())
      {
         nearbyBouncer = bouncer;
         break;
      }
   }

   return nearbyBouncer;
}


//-----------------------------------------------------------------------------
void Level::toggleMechanisms()
{
   for (auto& door : mDoors)
   {
      std::dynamic_pointer_cast<Door>(door)->toggle();
   }

   for (auto& lever : mLevers)
   {
      std::dynamic_pointer_cast<Lever>(lever)->toggle();
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


