#pragma once

// game
#include "ambientocclusion.h"
#include "camerasystem.h"
#include "constants.h"
#include "gamenode.h"
#include "imagelayer.h"
#include "luanode.h"
#include "portal.h"
#include "squaremarcher.h"
#include "joystick/gamecontrollerinfo.h"

// effects
#include "effects/raycastlight.h"
#include "effects/staticlight.h"

// sfml
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

// box2d
#include "Box2D/Box2D.h"

// std
#include <list>
#include <map>
#include <memory>

class Bouncer;
class ConveyorBelt;
class Door;
struct ExtraItem;
class Laser;
struct LevelDescription;
class LevelMap;
class MovingPlatform;
class TileMap;
struct TmxElement;
struct TmxObjectGroup;
class TmxParser;
struct TmxLayer;
struct TmxTileSet;


class Level : GameNode
{

public:

   struct Atmosphere
   {
      Atmosphere() = default;

      std::vector<int32_t> mMap;

      int32_t mMapOffsetX = 0;
      int32_t mMapOffsetY = 0;
      uint32_t mMapWidth = 0;
      uint32_t mMapHeight = 0;

      std::vector<std::vector<sf::Vertex>> mOutlines;
      std::vector<std::vector<b2Vec2>> mChains;
      std::shared_ptr<TileMap> mTileMap;

      AtmosphereTile getTileForPosition(const b2Vec2& playerPos) const;
   };


   struct Physics
   {
      uint32_t mGridWidth = 0;
      uint32_t mGridHeight = 0;
      uint32_t mGridSize = 0;

      std::vector<int32_t> mPhysicsMap;
   };


   Level();
   virtual ~Level();

   virtual void initialize();

   void createViews();
   void updateViews();

   void spawnEnemies();

   void draw(const std::shared_ptr<sf::RenderTexture>& window, bool screenshot);
   void drawRaycastLight(sf::RenderTarget& target);
   void drawParallaxMaps(sf::RenderTarget& target);
   void drawLayers(sf::RenderTarget& target, int from = 0, int to = 50);
   void drawAtmosphereLayer(sf::RenderTarget& target);
   void drawBlurLayer(sf::RenderTarget& target);
   void drawMap(sf::RenderTarget& target);

   sf::Vector2f getSize();

   void update(const sf::Time& dt);

   const std::shared_ptr<b2World>& getWorld() const;
   void setWorld(const std::shared_ptr<b2World>& world);

   std::map<b2Body *, b2Vec2 *> *getPointMap() ;
   std::map<b2Body*, size_t>* getPointSizeMap();

   static Level *getCurrentLevel();

   Portal* getNearbyPortal() const;
   Bouncer* getNearbyBouncer() const;

   void toggleDoor();
   void boom(float x, float y, float intensity);

   void reset();

   int getZ() const;
   void setZ(int z);

   const sf::Vector2f &getStartPosition() const;

   void drawStaticChains(sf::RenderTarget& target);

   std::shared_ptr<sf::View> getLevelView();

   std::string getDescriptionFilename() const;
   void setDescriptionFilename(const std::string &descriptionFilename);

   const Atmosphere& getPhysics() const;

   void initializeTextures();
   void initializeAtmosphereShader();
   void updateAtmosphereShader();
   void initializeGammaShader();
   void updateGammaShader();
   void initializeBlurShader();
   void updateBlurShader();

   bool isPhysicsPathClear(const sf::Vector2i& a, const sf::Vector2i& b) const;


protected:

   void updateBoom(const sf::Time& dt);

   void parsePolyline(
       float offsetX,
       float offsetY,
       const std::vector<sf::Vector2f> &poly
   );

   void addDebugRect(b2Body* body, float x, float y, float w, float h);

   void parseAtmosphereLayer(TmxLayer* layer, TmxTileSet* tileSet);
   void parseDynamicPhyicsLayer(TmxLayer* layer, TmxTileSet* tileSet);

   void parsePhysicsTiles(
      TmxLayer* layer,
      TmxTileSet* tileSet,
      const std::filesystem::path& basePath
   );

   void addPathsToWorld(
      int32_t offsetX,
      int32_t offsetY,
      const std::vector<SquareMarcher::Path>& paths,
      ObjectBehavior behavior
   );

   void load();

   void takeScreenshot(const std::string& basename, sf::RenderTexture &texture);

   std::shared_ptr<RaycastLight::LightInstance> deserializeRaycastLight(TmxObject* tmxObject);
   std::shared_ptr<StaticLight::LightInstance> deserializeStaticLight(TmxObject* tmxObject, TmxObjectGroup* objectGroup);
   std::shared_ptr<ImageLayer> deserializeImageLayer(TmxElement* tmxElement, const std::filesystem::path &levelPath);
   void deserializeParallaxMap(TmxLayer *layer);
   void loadTmx();


protected:

   std::shared_ptr<sf::RenderTexture> mLevelRenderTexture;
   std::shared_ptr<sf::RenderTexture> mLevelBackgroundRenderTexture;
   std::shared_ptr<sf::RenderTexture> mAtmosphereRenderTexture;
   std::shared_ptr<sf::RenderTexture> mBlurRenderTexture;
   std::shared_ptr<sf::RenderTexture> mBlurRenderTextureScaled;

   float mViewToTextureScale = 1.0f;
   std::shared_ptr<sf::View> mLevelView;
   std::shared_ptr<sf::View> mParallaxView[3];
   std::shared_ptr<sf::View> mMapView;

   std::map<std::string, int32_t> mScreenshotCounters;
   float mParallaxFactor[3] = {0.9f, 0.85f, 0.8f};
   float mViewWidth = 0.0f;
   float mViewHeight = 0.0f;
   float mBoomIntensity = 0.0f;
   float mBoomOffsetX = 0.0f;
   float mBoomOffsetY = 0.0f;
   float mCameraBoomIntensity = 1.0f;

   std::shared_ptr<LevelDescription> mDescription;

   std::vector<std::shared_ptr<TileMap>> mTileMaps;
   std::vector<std::shared_ptr<TileMap>> mParallaxMaps;

   std::vector<std::shared_ptr<LuaNode>> mEnemies;

   Atmosphere mAtmosphere;
   Physics mPhysics;

   sf::Vector2f mStartPosition;

   std::unique_ptr<TmxParser> mTmxParser;
   std::string mDescriptionFilename;

   std::unique_ptr<LevelMap> mMap;

   // mechanisms
   std::vector<Bouncer*> mBouncers;
   std::vector<ConveyorBelt*> mConveyorBelts;
   std::vector<Door*> mDoors;
   std::vector<Laser*> mLasers;
   std::vector<MovingPlatform*> mPlatforms;
   std::vector<Portal*> mPortals;

   // graphic effects
   std::shared_ptr<RaycastLight> mRaycastLight;
   std::shared_ptr<StaticLight> mStaticLight;

   AmbientOcclusion mAo;
   std::vector<std::shared_ptr<ImageLayer>> mImageLayers;

   bool mAtmosphereEnabled = false;
   sf::Shader mAtmosphereShader;
   sf::Texture mAtmosphereDistortionMap;
   sf::Shader mGammaShader;
   sf::Shader mBlurShader;

   // box2d

   // box2d world
   std::map<b2Body*, b2Vec2*> mPointMap;
   std::map<b2Body*, size_t> mPointCountMap;

   std::shared_ptr<b2World> mWorld;

   static Level* sCurrentLevel;
};

