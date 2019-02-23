#ifndef LEVEL_H
#define LEVEL_H

// game
#include "ambientocclusion.h"
#include "constants.h"
#include "door.h"
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
struct ExtraItem;
struct LevelDescription;
class MovingPlatform;
class TileMap;
struct TmxElement;
struct TmxObjectGroup;
class TmxParser;
struct TmxLayer;
struct TmxTileSet;


class Level : GameNode
{

  struct Physics
  {
    int* mMap = nullptr;
    int32_t mMapOffsetX = 0;
    int32_t mMapOffsetY = 0;
    int32_t mMapWidth = 0;
    int32_t mMapHeight = 0;
    std::vector<std::vector<sf::Vertex>> mOutlines;
    std::vector<std::vector<b2Vec2>> mChains;
    std::shared_ptr<TileMap> mTileMap;

    PhysicsTile getTileForPosition(const b2Vec2& playerPos) const;
  };


protected:

   std::shared_ptr<sf::View> mLevelView;
   std::shared_ptr<sf::View> mParallaxView[3];
   std::shared_ptr<sf::View> mMapView;

   float mParallaxFactor[3] = {0.9f, 0.85f, 0.8f};
   float mViewWidth = 0.0f;
   float mViewHeight = 0.0f;
   int32_t mLook = LookInactive;

   std::shared_ptr<LevelDescription> mDescription;

   std::map<b2Body*, b2Vec2*> mPointMap;
   std::map<b2Body*, int> mPointCountMap;

   std::vector<std::shared_ptr<TileMap>> mTileMaps;
   std::vector<std::shared_ptr<TileMap>> mParallaxMaps;

   std::vector<MovingPlatform*> mPlatforms;
   std::vector<Door*> mDoors;
   std::vector<Portal*> mPortals;
   std::vector<Bouncer*> mBouncers;
   std::vector<ConveyorBelt*> mConveyorBelts;
   std::vector<std::shared_ptr<LuaNode>> mEnemies;

   Physics mPhysics;

   sf::Vector2f mStartPosition;
   sf::Vector2f mLookVector;

   std::unique_ptr<TmxParser> mTmxParser;
   std::string mDescriptionFilename;

   std::shared_ptr<RaycastLight> mRaycastLight;
   std::shared_ptr<StaticLight> mStaticLight;

   AmbientOcclusion mAo;
   std::vector<std::shared_ptr<ImageLayer>> mImageLayers;
   GameControllerInfo mJoystickInfo;

   // box2d

   // box2d world
   b2World* mWorld;

   static Level* sCurrentLevel;


public:

   Level();
   virtual ~Level();

   virtual void initialize();

   void createViews();
   void updateViews();

   void spawnEnemies();

   void draw(sf::RenderTarget& target);
   void drawRaycastLight(sf::RenderTarget& target);
   void drawParallaxMaps(sf::RenderTarget& target);
   void drawLayers(sf::RenderTarget& target, int from = 0, int to = 50);
   void drawAtmosphereLayer(sf::RenderTarget& target);
   void drawMap(sf::RenderTarget& target);

   sf::Vector2f getSize();

   void update(float dt);

   b2World *getWorld() const;
   void setWorld(b2World *world);

   std::map<b2Body *, b2Vec2 *> *getPointMap() ;
   std::map<b2Body *, int>* getPointSizeMap();

   static Level *getCurrentLevel();

   Portal* getNearbyPortal() const;
   void toggleDoor();

   void reset();

   int getZ() const;
   void setZ(int z);

   const sf::Vector2f &getStartPosition() const;

   void drawStaticChains(std::shared_ptr<sf::RenderWindow>& window);

   void updateLookVector();
   void updateLookState(Look look, bool enable);

   std::shared_ptr<sf::View> getLevelView();

   GameControllerInfo getJoystickInfo() const;
   void setJoystickInfo(const GameControllerInfo &joystickInfo);

   std::string getDescriptionFilename() const;
   void setDescriptionFilename(const std::string &descriptionFilename);

   const Physics& getPhysics() const;


private:

   void parsePolyline(
       float offsetX,
       float offsetY,
       const std::vector<sf::Vector2f> &poly
   );

   void addDebugRect(b2Body* body, float x, float y, float w, float h);

   bool isTileCombinable(int tileNumber) const;
   bool isTileOneSided(int tileNumber) const;
   bool isTileDeadly(int tileNumber) const;
   bool isTileSolid(int tileNumber) const;

   b2Vec2 * createShape(
      int tile,
      unsigned int i,
      unsigned int j,
      int tileWidth,
      int tileHeight,
      int horizontalSpan,
      int& polyCount
   );

   void parsePhysicsLayer(TmxLayer* layer, TmxTileSet* tileSet);
   void parseDynamicPhyicsLayer(TmxLayer* layer, TmxTileSet* tileSet);

   void loadLevel();
   bool isControllerUsed() const;

   std::shared_ptr<RaycastLight::LightInstance> deserializeRaycastLight(TmxObject* tmxObject);
   std::shared_ptr<StaticLight::LightInstance> deserializeStaticLight(TmxObject* tmxObject, TmxObjectGroup* objectGroup);
   std::shared_ptr<ImageLayer> deserializeImageLayer(TmxElement* tmxElement, const std::filesystem::path &levelPath);
   void deserializeParallaxMap(TmxLayer *layer);
   void loadTmx();
};

#endif // LEVEL_H
