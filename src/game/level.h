#pragma once

// game
#include "ambientocclusion.h"
#include "atmosphere.h"
#include "boomeffect.h"
#include "camerasystem.h"
#include "constants.h"
#include "enemy.h"
#include "framework/joystick/gamecontrollerinfo.h"
#include "gamenode.h"
#include "imagelayer.h"
#include "luanode.h"
#include "mechanisms/portal.h"
#include "physics/physics.h"
#include "room.h"
#include "shaders/atmosphereshader.h"
#include "shaders/blurshader.h"
#include "shaders/deathshader.h"
#include "shaders/gammashader.h"
#include "shaderlayer.h"
#include "squaremarcher.h"

// effects
#include "effects/lightsystem.h"
#include "effects/smokeeffect.h"
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
class Rope;
class SpikeBall;
class Spikes;
class TileMap;
struct TmxElement;
struct TmxObjectGroup;
class TmxParser;
struct TmxLayer;
struct TmxTileSet;


class Level : public GameNode
{

public:

   Level();
   virtual ~Level();

   virtual void initialize();
   void initializeTextures();
   void reset();
   void resetDeathShader();

   void createViews();

   void update(const sf::Time& dt);
   void updateViews();
   void updateCameraSystem(const sf::Time& dt);

   void spawnEnemies();

   void draw(const std::shared_ptr<sf::RenderTexture>& window, bool screenshot);
   void drawLightAndShadows(sf::RenderTarget& target);
   void drawParallaxMaps(sf::RenderTarget& target);
   void drawLayers(sf::RenderTarget& color, sf::RenderTarget& normal, int32_t from, int32_t to);
   void drawAtmosphereLayer(sf::RenderTarget& target);
   void drawBlurLayer(sf::RenderTarget& target);
   void drawNormalMap();
   void drawLightMap();
   void drawPlayer(sf::RenderTarget& color, sf::RenderTarget& normal);

   const std::shared_ptr<b2World>& getWorld() const;

   std::map<b2Body*, b2Vec2*>* getPointMap();
   std::map<b2Body*, size_t>* getPointSizeMap();

   static Level* getCurrentLevel();

   std::shared_ptr<Portal> getNearbyPortal();
   std::shared_ptr<Bouncer> getNearbyBouncer();

   void toggleMechanisms();

   const sf::Vector2f& getStartPosition() const;

   void drawStaticChains(sf::RenderTarget& target);

   std::string getDescriptionFilename() const;
   void setDescriptionFilename(const std::string &description_filename);

   const Atmosphere& getPhysics() const;

   bool isPhysicsPathClear(const sf::Vector2i& a, const sf::Vector2i& b) const;

   BoomEffect& getBoomEffect();

   const std::shared_ptr<LightSystem>& getLightSystem() const;

   const std::shared_ptr<sf::View>& getLevelView() const;


protected:

   void addDebugRect(b2Body* body, float x, float y, float w, float h);

   void parsePhysicsTiles(
         TmxLayer* layer,
      TmxTileSet* tileSet,
      const std::filesystem::path& basePath
   );

   void addPathsToWorld(
      int32_t offsetX,
      int32_t offsetY,
      const std::vector<SquareMarcher::Path>& paths,
      ObjectType behavior
   );

   void addChainToWorld(
      const std::vector<b2Vec2>& chain,
      ObjectType behavior
   );

   void addDebugOutlines(
      int32_t offsetX,
      int32_t offsetY,
      std::vector<sf::Vector2f> positions,
      ObjectType behavior
   );

   void parseObj(
      TmxLayer* layer,
      ObjectType behavior,
      const std::filesystem::path& path
   );

   void load();
   void loadTmx();
   void loadCheckpoint();

   void deserializeParallaxMap(TmxLayer* layer);

   void takeScreenshot(const std::string& basename, sf::RenderTexture &texture);
   void updatePlayerLight();

   void drawDebugInformation();
   void displayTextures();
   void drawGlowLayer();
   void drawGlowSprite();


   std::vector<std::shared_ptr<Room>> _rooms;
   std::shared_ptr<Room> _room_current;
   std::shared_ptr<Room> _room_previous;

   std::shared_ptr<sf::RenderTexture> _render_texture_level;
   std::shared_ptr<sf::RenderTexture> _render_texture_level_background;
   std::shared_ptr<sf::RenderTexture> _render_texture_lighting;
   std::shared_ptr<sf::RenderTexture> _render_texture_normal;
   std::shared_ptr<sf::RenderTexture> _render_texture_deferred;
   std::vector<std::shared_ptr<sf::RenderTexture>> _render_textures;

   float _view_to_texture_scale = 1.0f;
   std::shared_ptr<sf::View> _level_view;
   std::shared_ptr<sf::View> _parallax_view[3];

   std::map<std::string, int32_t> _screenshot_counters;
   float _parallax_factors[3] = {0.9f, 0.85f, 0.8f};
   float _view_width = 0.0f;
   float _view_height = 0.0f;

   std::shared_ptr<LevelDescription> _description;
   std::string _description_filename;

   std::vector<std::shared_ptr<TileMap>> _tile_maps;
   std::vector<std::shared_ptr<TileMap>> _parallax_maps;

   std::vector<std::shared_ptr<LuaNode>> _enemies;
   std::map<std::string, Enemy> _enemy_data_from_tmx_layer;

   Atmosphere _atmosphere;
   Physics _physics;

   sf::Vector2f _start_position;

   std::unique_ptr<TmxParser> _tmx_parser;
   std::vector<TmxElement*> _tmx_elements;

   std::unique_ptr<LevelMap> _map;

   // mechanisms
   std::vector<std::vector<std::shared_ptr<GameMechanism>>*> _mechanisms;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_bouncers;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_conveyor_belts;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_crushers;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_death_blocks;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_doors;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_fans;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_lasers;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_levers;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_platforms;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_portals;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_ropes;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_spike_balls;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_spikes;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_moveable_boxes;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_dialogues;

   // graphic effects
   BoomEffect _boom_effect;
   std::shared_ptr<LightSystem> _light_system;
   std::shared_ptr<StaticLight> _static_light;
   std::shared_ptr<LightSystem::LightInstance> _player_light;
   std::vector<std::shared_ptr<SmokeEffect>> _smoke_effect;

   AmbientOcclusion _ambient_occlusion;
   std::vector<std::shared_ptr<ImageLayer>> _image_layers;
   std::vector<std::shared_ptr<ShaderLayer>> _shader_layers;

   std::unique_ptr<AtmosphereShader> _atmosphere_shader;
   std::unique_ptr<BlurShader> _blur_shader;
   std::unique_ptr<GammaShader> _gamme_shader;
   std::unique_ptr<DeathShader> _death_shader;
   bool _screenshot = false;

   // box2d
   std::map<b2Body*, b2Vec2*> _point_map;
   std::map<b2Body*, size_t> _point_count_map;

   std::shared_ptr<b2World> _world = nullptr;
   std::vector<std::vector<b2Vec2>> _world_chains;

   static Level* __current_level;
};

