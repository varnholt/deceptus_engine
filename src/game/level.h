#pragma once

// game
#include "framework/joystick/gamecontrollerinfo.h"
#include "game/ambientocclusion.h"
#include "game/atmosphere.h"
#include "game/boomeffect.h"
#include "game/camerasystem.h"
#include "game/constants.h"
#include "game/effects/lightsystem.h"
#include "game/gamenode.h"
#include "game/levelscript.h"
#include "game/mechanisms/imagelayer.h"
#include "game/mechanisms/portal.h"
#include "game/mechanisms/staticlight.h"
#include "game/parallaxlayer.h"
#include "game/physics/physics.h"
#include "game/room.h"
#include "game/shaders/atmosphereshader.h"
#include "game/shaders/blurshader.h"
#include "game/shaders/gammashader.h"
#include "game/squaremarcher.h"
#include "game/tmxenemy.h"
#include "game/volumeupdater.h"

// sfml
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

// box2d
#include <box2d/box2d.h>

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
   void loadStartPosition();

   void saveState();

   void createViews();

   void update(const sf::Time& dt);
   void updateViews();
   void updateMechanismVolumes();
   void updateCameraSystem(const sf::Time& dt);

   void spawnEnemies();

   void draw(const std::shared_ptr<sf::RenderTexture>& window, bool screenshot);

   const std::shared_ptr<b2World>& getWorld() const;
   const sf::Vector2f& getStartPosition() const;

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
   const std::vector<std::shared_ptr<GameMechanism>>& getExtras() const;

   bool isDirty() const;
   void setLoadingMode(LoadingMode loading_mode);

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

   std::vector<std::shared_ptr<GameMechanism>>
   searchMechanisms(const std::string& regexp, const std::optional<std::string>& group = std::nullopt);

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

   std::shared_ptr<LevelDescription> _description;
   std::string _description_filename;

   std::vector<std::shared_ptr<TileMap>> _tile_maps;

   std::unordered_map<std::string, TmxEnemy> _enemy_data_from_tmx_layer;

   Atmosphere _atmosphere;
   Physics _physics;
   sf::Vector2f _start_position;

   std::vector<std::unique_ptr<ParallaxLayer>> _parallax_layers;

   // mechanisms
   std::unique_ptr<VolumeUpdater> _volume_updater;
   std::unordered_map<std::string, std::vector<std::shared_ptr<GameMechanism>>*> _mechanisms_map;
   std::vector<std::vector<std::shared_ptr<GameMechanism>>*> _mechanisms_list;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_blocking_rects;
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
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_extras;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_fans;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_fireflies;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_info_overlay;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_interaction_help;
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
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_smoke_effect;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_sound_emitters;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_spike_balls;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_spike_blocks;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_spikes;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_text_layers;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_treasure_chests;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_static_lights;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_water_damage;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_water_surface;
   std::vector<std::shared_ptr<GameMechanism>> _mechanism_weather;

   // graphic effects
   BoomEffect _boom_effect;
   std::shared_ptr<LightSystem> _light_system;
   std::shared_ptr<LightSystem::LightInstance> _player_light;

   AmbientOcclusion _ambient_occlusion;
   std::vector<std::shared_ptr<ImageLayer>> _image_layers;

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
