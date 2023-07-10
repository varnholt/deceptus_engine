#pragma once

// game
#include "ambientocclusion.h"
#include "atmosphere.h"
#include "boomeffect.h"
#include "camerasystem.h"
#include "constants.h"
#include "framework/joystick/gamecontrollerinfo.h"
#include "gamenode.h"
#include "imagelayer.h"
#include "luanode.h"
#include "mechanisms/portal.h"
#include "objectupdater.h"
#include "physics/physics.h"
#include "room.h"
#include "shaders/atmosphereshader.h"
#include "shaders/blurshader.h"
#include "shaders/gammashader.h"
#include "squaremarcher.h"
#include "tmxenemy.h"

// effects
#include "effects/lightsystem.h"
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
class IngameMenuMap;
class TmxParser;
struct ParseData;

/*! \brief Implements all level-related logic
 *         That includes deserialization of all level-data, updating mechanisms and rendering them.
 *         Level physics information is transformed into a Box2D representation that handles collision detection.
 *
 *  The Level implementation loads all information from TMX in the load() function, all tile maps and animations,
 *  mechanisms, the Box2D world and LUA-based enemies are updated inside update(). Drawing of all effects and tile maps
 *  happens inside draw().
 */
class Level : public GameNode
{
public:
   Level();
   virtual ~Level();

   virtual void initialize();
   void initializeTextures();
   void reset();

   void saveState();

   void createViews();

   void update(const sf::Time& dt);
   void updateViews();
   void updateObjectUpdater();
   void updateCameraSystem(const sf::Time& dt);

   void spawnEnemies();

   void draw(const std::shared_ptr<sf::RenderTexture>& window, bool screenshot);

   const std::shared_ptr<b2World>& getWorld() const;
   const sf::Vector2f& getStartPosition() const;

   const std::unordered_map<void*, b2Vec2*>& getPointMap() const;
   const std::unordered_map<void*, size_t>& getPointSizeMap() const;

   const std::vector<std::shared_ptr<GameMechanism>>& getCheckpoints() const;

   void toggleMechanisms();

   std::string getDescriptionFilename() const;
   void setDescriptionFilename(const std::string& description_filename);

   const Atmosphere& getAtmosphere() const;

   bool isPhysicsPathClear(const sf::Vector2i& a_tl, const sf::Vector2i& b_tl) const;

   BoomEffect& getBoomEffect();

   const std::shared_ptr<LightSystem>& getLightSystem() const;
   const std::shared_ptr<sf::View>& getLevelView() const;

   static Level* getCurrentLevel();
   void syncRoom();

   const std::vector<std::shared_ptr<GameMechanism>>& getBouncers() const;
   const std::vector<std::shared_ptr<GameMechanism>>& getPortals() const;

protected:
   void addDebugRect(void* body, float x, float y, float w, float h);

   void parsePhysicsTiles(
      const std::shared_ptr<TmxLayer>& layer,
      const std::shared_ptr<TmxTileSet>& tileSet,
      const std::filesystem::path& basePath
   );

   void addPathsToWorld(int32_t offsetX, int32_t offsetY, const std::vector<SquareMarcher::Path>& paths, ObjectType behavior);

   void addChainToWorld(const std::vector<b2Vec2>& chain, ObjectType behavior);

   void parseObj(const std::shared_ptr<TmxLayer>& layer, ObjectType behavior, const std::filesystem::path& path);

   bool load();
   void loadTmx();
   void loadState();

   void regenerateLevelPaths(
      const std::shared_ptr<TmxLayer>& layer,
      const std::shared_ptr<TmxTileSet>& tileset,
      const std::filesystem::path& base_path,
      const SquareMarcher& square_marcher,
      ParseData* parse_data,
      auto path_solid_optimized
   );

   void deserializeParallaxMap(const std::shared_ptr<TmxLayer>& layer, const std::shared_ptr<TileMap>& tile_map);

   void takeScreenshot(const std::string& basename, sf::RenderTexture& texture);
   void updatePlayerLight();
   void updateRoom();

   void drawLightAndShadows(sf::RenderTarget& target);
   void drawParallaxMaps(sf::RenderTarget& target, int32_t z_index);
   void drawLayers(sf::RenderTarget& color, sf::RenderTarget& normal, int32_t from, int32_t to);
   void drawAtmosphereLayer(sf::RenderTarget& target);
   void drawBlurLayer(sf::RenderTarget& target);
   void drawNormalMap();
   void drawLightMap();
   void drawPlayer(sf::RenderTarget& color, sf::RenderTarget& normal);
   void drawStaticChains(sf::RenderTarget& target);
   void drawDebugInformation();
   void displayTextures();
   void drawGlowLayer();
   void drawGlowSprite();

   std::vector<std::shared_ptr<Room>> _rooms;
   std::shared_ptr<Room> _room_current;
   std::shared_ptr<Room> _room_previous;
   bool _room_synced = false;

   std::shared_ptr<sf::RenderTexture> _render_texture_level;
   std::shared_ptr<sf::RenderTexture> _render_texture_level_background;
   std::shared_ptr<sf::RenderTexture> _render_texture_lighting;
   std::shared_ptr<sf::RenderTexture> _render_texture_normal;
   std::shared_ptr<sf::RenderTexture> _render_texture_deferred;
   std::vector<std::shared_ptr<sf::RenderTexture>> _render_textures;

   float _view_to_texture_scale = 1.0f;
   std::shared_ptr<sf::View> _level_view;

   std::map<std::string, int32_t> _screenshot_counters;
   float _view_width = 0.0f;
   float _view_height = 0.0f;

   std::shared_ptr<LevelDescription> _description;
   std::string _description_filename;

   std::vector<std::shared_ptr<TileMap>> _tile_maps;

   std::unordered_map<std::string, TmxEnemy> _enemy_data_from_tmx_layer;

   Atmosphere _atmosphere;
   Physics _physics;
   sf::Vector2f _start_position;

   // parallax (move to separate mechanism!)
   struct ParallaxLayer
   {
      bool _used = false;
      int32_t _z_index = 0;
      sf::Vector2f _factor;
      sf::Vector2f _offset;
      sf::Vector2f _error;
      std::shared_ptr<sf::View> _view;
      std::shared_ptr<TileMap> _tile_map;
   };

   std::array<ParallaxLayer, 3> _parallax_layers;

   // mechanisms
   std::unique_ptr<ObjectUpdater> _object_updater;
   std::unordered_map<std::string, std::vector<std::shared_ptr<GameMechanism>>*> _mechanisms_map;
   std::vector<std::vector<std::shared_ptr<GameMechanism>>*> _mechanisms_list;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_bouncers;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_bubble_cubes;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_checkpoints;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_collapsing_platforms;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_controller_help;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_conveyor_belts;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_crushers;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_damage_rects;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_death_blocks;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_dialogues;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_doors;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_dust;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_fans;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_lasers;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_levers;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_moveable_boxes;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_on_off_blocks;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_platforms;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_portals;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_ropes;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_rotating_blades;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_sensor_rects;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_shader_layers;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_sound_emitters;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_smoke_effect;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_spike_balls;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_spike_blocks;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_spikes;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_water_surface;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_weather;

   // graphic effects
   BoomEffect _boom_effect;
   std::shared_ptr<LightSystem> _light_system;
   std::shared_ptr<StaticLight> _static_light;
   std::shared_ptr<LightSystem::LightInstance> _player_light;

   AmbientOcclusion _ambient_occlusion;
   std::vector<std::shared_ptr<ImageLayer>> _image_layers;

   std::unique_ptr<AtmosphereShader> _atmosphere_shader;
   std::unique_ptr<BlurShader> _blur_shader;
   std::unique_ptr<GammaShader> _gamma_shader;
   bool _screenshot = false;

   // box2d
   std::unordered_map<void*, b2Vec2*> _point_map;
   std::unordered_map<void*, size_t> _point_count_map;

   std::shared_ptr<b2World> _world = nullptr;
   std::vector<std::vector<b2Vec2>> _world_chains;
   Winding _winding = Winding::Clockwise;

   // file watcher
   std::thread _file_watcher_thread;
   bool _file_watcher_thread_active{true};

   static Level* __current_level;
};
