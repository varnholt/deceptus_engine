#pragma once

// game
#include "ambientocclusion.h"
#include "atmosphere.h"
#include "atmosphereshader.h"
#include "blurshader.h"
#include "boomeffect.h"
#include "camerasystem.h"
#include "constants.h"
#include "gamenode.h"
#include "gammashader.h"
#include "imagelayer.h"
#include "luanode.h"
#include "physics.h"
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
class MoveableBox;
class MovingPlatform;
class TileMap;
class SpikeBall;
class Spikes;
struct TmxElement;
struct TmxObjectGroup;
class TmxParser;
struct TmxLayer;
struct TmxTileSet;


class Level : GameNode
{

public:

   Level();
   virtual ~Level();

   virtual void initialize();

   void createViews();
   void updateViews();

   void spawnEnemies();

   void draw(const std::shared_ptr<sf::RenderTexture>& window, bool screenshot);
   void drawRaycastLight(sf::RenderTarget& target);
   void drawParallaxMaps(sf::RenderTarget& target);
   void drawLayers(sf::RenderTarget& target, int32_t from, int32_t to);
   void drawAtmosphereLayer(sf::RenderTarget& target);
   void drawBlurLayer(sf::RenderTarget& target);
   void drawMap(sf::RenderTarget& target);

   void update(const sf::Time& dt);

   const std::shared_ptr<b2World>& getWorld() const;

   std::map<b2Body *, b2Vec2 *>* getPointMap() ;
   std::map<b2Body*, size_t>* getPointSizeMap();

   static Level* getCurrentLevel();

   std::shared_ptr<Portal> getNearbyPortal();
   std::shared_ptr<Bouncer> getNearbyBouncer();

   void toggleMechanisms();
   void reset();

   const sf::Vector2f& getStartPosition() const;

   void drawStaticChains(sf::RenderTarget& target);

   std::shared_ptr<sf::View> getLevelView();

   std::string getDescriptionFilename() const;
   void setDescriptionFilename(const std::string &descriptionFilename);

   const Atmosphere& getPhysics() const;

   void initializeTextures();

   bool isPhysicsPathClear(const sf::Vector2i& a, const sf::Vector2i& b) const;

   BoomEffect& getBoomEffect();


protected:

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

   void deserializeParallaxMap(TmxLayer* layer);
   void loadTmx();

   std::shared_ptr<sf::RenderTexture> mLevelRenderTexture;
   std::shared_ptr<sf::RenderTexture> mLevelBackgroundRenderTexture;
   std::vector<std::shared_ptr<sf::RenderTexture>> mRenderTextures;

   float mViewToTextureScale = 1.0f;
   std::shared_ptr<sf::View> mLevelView;
   std::shared_ptr<sf::View> mParallaxView[3];
   std::shared_ptr<sf::View> mMapView;

   std::map<std::string, int32_t> mScreenshotCounters;
   float mParallaxFactors[3] = {0.9f, 0.85f, 0.8f};
   float mViewWidth = 0.0f;
   float mViewHeight = 0.0f;

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
   std::vector<std::vector<std::shared_ptr<GameMechanism>>*> mMechanisms;
   std::vector<std::shared_ptr<GameMechanism>> mBouncers;
   std::vector<std::shared_ptr<GameMechanism>> mConveyorBelts;
   std::vector<std::shared_ptr<GameMechanism>> mDoors;
   std::vector<std::shared_ptr<GameMechanism>> mFans;
   std::vector<std::shared_ptr<GameMechanism>> mLasers;
   std::vector<std::shared_ptr<GameMechanism>> mLevers;
   std::vector<std::shared_ptr<GameMechanism>> mPlatforms;
   std::vector<std::shared_ptr<GameMechanism>> mPortals;
   std::vector<std::shared_ptr<GameMechanism>> mSpikeBalls;
   std::vector<std::shared_ptr<GameMechanism>> mSpikes;
   std::vector<std::shared_ptr<GameMechanism>> mMoveableBoxes;

   // graphic effects
   BoomEffect mBoomEffect;
   std::shared_ptr<RaycastLight> mRaycastLight;
   std::shared_ptr<StaticLight> mStaticLight;
   std::shared_ptr<RaycastLight::LightInstance> mPlayerLight;

   AmbientOcclusion mAo;
   std::vector<std::shared_ptr<ImageLayer>> mImageLayers;

   std::unique_ptr<AtmosphereShader> mAtmosphereShader;
   std::unique_ptr<BlurShader> mBlurShader;
   std::unique_ptr<GammaShader> mGammaShader;

   // box2d

   // box2d world
   std::map<b2Body*, b2Vec2*> mPointMap;
   std::map<b2Body*, size_t> mPointCountMap;

   std::shared_ptr<b2World> mWorld;

   static Level* sCurrentLevel;
};

