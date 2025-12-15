#pragma once

// game
#include "game/audio/volumeupdater.h"
#include "game/constants.h"
#include "game/effects/boomeffect.h"
#include "game/effects/lightsystem.h"
#include "game/layers/ambientocclusion.h"
#include "game/layers/parallaxlayer.h"
#include "game/level/atmosphere.h"
#include "game/level/gamemechanismregistry.h"
#include "game/level/gamenode.h"
#include "game/level/leveldescription.h"
#include "game/level/levelscript.h"
#include "game/level/room.h"
#include "game/level/tmxenemy.h"
#include "game/mechanisms/imagelayer.h"
#include "game/mechanisms/portal.h"
#include "game/physics/physics.h"
#include "game/physics/squaremarcher.h"
#include "game/shaders/atmosphereshader.h"
#include "game/shaders/blurshader.h"
#include "game/shaders/gammashader.h"

// sfml
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

// box2d
#include "box2d/box2d.h"

// std
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
   void loadStartPosition();

   void saveState();

   void createViews();

   void update(const sf::Time& dt);
   void updateViews();
   void updateMechanismVolumes();
   void updateCameraSystem(const sf::Time& dt);

   void zoomIn();
   void zoomOut();
   void zoomBy(float delta);
   void zoomReset();

   void spawnEnemies();

   void draw(const std::shared_ptr<sf::RenderTexture>& window, bool screenshot);

   const std::shared_ptr<b2World>& getWorld() const;
   const sf::Vector2f& getStartPosition() const;

   std::string getDescriptionFilename() const;
   void setDescriptionFilename(const std::string& description_filename);

   const Atmosphere& getAtmosphere() const;

   bool isPhysicsPathClear(const sf::Vector2i& a_tl, const sf::Vector2i& b_tl) const;

   BoomEffect& getBoomEffect();

   const std::shared_ptr<LightSystem>& getLightSystem() const;
   const std::shared_ptr<sf::View>& getLevelView() const;

   static Level* getCurrentLevel();
   void syncRoom();

   bool isDirty() const;
   void setLoadingMode(LoadingMode loading_mode);

   const GameMechanismRegistry& getMechanismRegistry() const;

protected:
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
   void loadSaveState();
   void loadLevelScript();

   void regenerateLevelPaths(
      const std::shared_ptr<TmxLayer>& layer,
      const std::shared_ptr<TmxTileSet>& tileset,
      const std::filesystem::path& base_path,
      ParseData* parse_data,
      auto path_solid_optimized
   );

   void assignMechanismsToRooms();

   void takeScreenshot(const std::string& basename, sf::RenderTexture& texture);
   void updatePlayerLight();
   void updateRoom();

   void drawParallaxMaps(sf::RenderTarget& target, int32_t z_index);
   void drawLayers(sf::RenderTarget& color, sf::RenderTarget& normal, int32_t from, int32_t to);
   void drawAtmosphereLayer();
   void drawBlurLayer(sf::RenderTarget& target);
   void drawLightMap();
   void drawPlayer(sf::RenderTarget& color, sf::RenderTarget& normal);
   void drawStaticChains(sf::RenderTarget& target);
   void drawDebugInformation();
   void displayFinalTextures();
   void drawGlowLayer();
   void drawGlowSprite();

   std::vector<std::shared_ptr<Room>> _rooms;
   LevelScript _level_script;

   std::shared_ptr<sf::RenderTexture> _render_texture_level;
   std::shared_ptr<sf::RenderTexture> _render_texture_level_background;
   std::shared_ptr<sf::RenderTexture> _render_texture_lighting;
   std::shared_ptr<sf::RenderTexture> _render_texture_normal;
   std::shared_ptr<sf::RenderTexture> _render_texture_normal_tmp;
   std::shared_ptr<sf::RenderTexture> _render_texture_deferred;
   std::vector<std::shared_ptr<sf::RenderTexture>> _render_textures;

   float _view_to_texture_scale = 1.0f;
   std::shared_ptr<sf::View> _level_view;

   std::map<std::string, int32_t> _screenshot_counters;
   float _view_width = 0.0f;
   float _view_height = 0.0f;
   float _zoom_factor = 1.0f;

   std::shared_ptr<LevelDescription> _description;
   std::string _description_filename;

   std::vector<std::shared_ptr<TileMap>> _tile_maps;

   std::unordered_map<std::string, TmxEnemy> _enemy_data_from_tmx_layer;

   Atmosphere _atmosphere;
   Physics _physics;
   sf::Vector2f _start_position;

   std::vector<std::unique_ptr<ParallaxLayer>> _parallax_layers;

   GameMechanismRegistry _mechanism_registry;
   std::unique_ptr<VolumeUpdater> _volume_updater;

   // graphic effects
   BoomEffect _boom_effect;
   std::shared_ptr<LightSystem> _light_system;
   std::shared_ptr<LightSystem::LightInstance> _player_light;
   std::unique_ptr<AmbientOcclusion> _ambient_occlusion;
   std::unique_ptr<AtmosphereShader> _atmosphere_shader;
   std::unique_ptr<BlurShader> _blur_shader;
   std::unique_ptr<GammaShader> _gamma_shader;
   bool _screenshot = false;

   // box2d
   std::shared_ptr<b2World> _world = nullptr;
   std::vector<std::vector<b2Vec2>> _world_chains;
   Winding _winding = Winding::Clockwise;

   // file watcher and re-generation
   std::thread _file_watcher_thread;
   bool _file_watcher_thread_active{true};
   bool _dirty{false};
   LoadingMode _loading_mode{LoadingMode::Standard};

   static Level* __current_level;
};
