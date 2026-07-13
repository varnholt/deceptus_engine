#pragma once

// game
#include "framework/tools/sfmlshader.h"
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
#include "game/level/levelinterface.h"
#include "game/level/levelscript.h"
#include "game/level/room.h"
#include "game/level/tmxenemy.h"
#include "game/mechanisms/imagelayer.h"
#include "game/mechanisms/portal.h"
#include "game/physics/physics.h"
#include "game/physics/squaremarcher.h"
#include "game/rendering/rendertargets.h"
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
#ifdef __EMSCRIPTEN__
#include <optional>
#endif

#ifdef DEVELOPMENT_MODE
#include <vector>
#include "game/debug/mechanismsample.h"
#endif

class Bouncer;
class IngameMenuMap;
class TmxParser;
struct ParseData;

/// \brief manages a playable level including tmx loading, physics, mechanisms, camera, and rendering.
class Level : public GameNode, public LevelInterface
{
public:
   /// \brief disables default construction because render targets are required.
   Level() = delete;

   /// \brief creates a level and initializes core systems such as box2d world, shaders, and light system.
   /// \param render_targets shared render textures used by the level rendering pipeline.
   explicit Level(const RenderTargets& render_targets);

   /// \brief stops background watchers and removes active enemy timers before destruction.
   virtual ~Level();

   /// \brief loads level description, tmx content, save state, enemies, scripts, and render view setup.
   virtual void initialize();

   /// \brief resets runtime door state in the mechanism registry.
   void reset();

   /// \brief computes the player spawn position in pixels from the description tile start position.
   void loadStartPosition() override;

   /// \brief serializes mechanism state into the current save state entry for this level.
   void saveState() override;

   /// \brief creates or resets main, parallax, and image-layer views from current game configuration.
   void createViews();

   /// \brief advances physics, mechanisms, enemies, scripts, camera, and audio volumes for one frame.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt);

   /// \brief recalculates level, parallax, and image-layer views from camera position, panorama, and zoom.
   void updateViews();

   /// \brief refreshes mechanism audio volume inputs with the current player position.
   void updateMechanismVolumes();

#ifdef DEVELOPMENT_MODE
   /// \brief enables or disables per-mechanism cpu timing collection.
   /// \param enabled true to record update and draw costs per mechanism type.
   void setMechanismProfilingEnabled(bool enabled);

   /// \brief returns a snapshot of per-mechanism cpu costs sorted by total cost descending.
   /// \param top_n maximum number of entries to return.
   std::vector<MechanismSample> getMechanismTimings(int32_t top_n) const;
#endif

   /// \brief advances active room and camera behavior, including room locks, transitions, and zoom.
   /// \param dt elapsed frame time.
   void updateCameraSystem(const sf::Time& dt);

   /// \brief decreases camera zoom factor slightly to zoom in.
   void zoomIn() override;

   /// \brief increases camera zoom factor slightly to zoom out.
   void zoomOut() override;

   /// \brief modifies camera zoom factor by a scaled delta and clamps it to a safe range.
   /// \param delta signed user zoom input.
   void zoomBy(float delta) override;

   /// \brief resets camera zoom factor to 1.0.
   void zoomReset() override;

   /// \brief spawns tmx-defined enemies and initializes their lua nodes and runtime properties.
   void spawnEnemies();

   /// \brief renders the complete level frame through atmosphere, layer, lighting, and gamma passes.
   /// \param window final render target that receives the composited scene.
   /// \param screenshot when true, enables debug screenshot dumps of intermediate textures.
   void draw(const std::shared_ptr<sf::RenderTexture>& window, bool screenshot);

   /// \brief returns the level's shared box2d world instance.
   /// \return shared pointer reference to the active box2d world.
   const std::shared_ptr<b2World>& getWorld() const override;

   /// \brief returns the current player start or checkpoint spawn position in pixels.
   /// \return spawn position in pixel coordinates.
   const sf::Vector2f& getStartPosition() const override;

   /// \brief returns the configured level description file path.
   /// \return level description filename.
   std::string getDescriptionFilename() const;

   /// \brief sets the level description file path to load during initialize().
   /// \param description_filename path to the level json description file.
   void setDescriptionFilename(const std::string& description_filename);

   /// \brief returns atmosphere layer data used by atmosphere rendering and tile queries.
   /// \return immutable reference to the atmosphere state.
   const Atmosphere& getAtmosphere() const override;

   /// \brief tests whether a straight tile-to-tile line is unobstructed in the physics occupancy grid.
   /// \param a_tl start tile coordinate.
   /// \param b_tl end tile coordinate.
   /// \return true when the line does not cross a blocking physics cell and both points are in bounds.
   bool isPhysicsPathClear(const sf::Vector2i& a_tl, const sf::Vector2i& b_tl) const override;

   /// \brief returns the screen shake and boom effect controller.
   /// \return mutable boom effect instance.
   BoomEffect& getBoomEffect() override;

   /// \brief returns the raycast light system used by deferred lighting.
   /// \return shared pointer reference to the light system.
   const std::shared_ptr<LightSystem>& getLightSystem() const override;

   /// \brief returns the ambient player light instance.
   /// \return shared pointer to the player light, or nullptr if not created.
   const std::shared_ptr<LightSystem::LightInstance>& getPlayerLight() const override;

   /// \brief returns the current gameplay camera view.
   /// \return shared pointer reference to the level view.
   const std::shared_ptr<sf::View>& getLevelView() const override;

   /// \brief synchronizes room updater and camera room lock to the player's current room immediately.
   void syncRoom();

   /// \brief reports whether file watching detected a modified level source file.
   /// \return true when the level content changed on disk and should be reloaded.
   bool isDirty() const;

   /// \brief sets how level loading handles generated physics artifacts.
   /// \param loading_mode loading mode controlling cleanup and regeneration behavior.
   void setLoadingMode(LoadingMode loading_mode);

   /// \brief returns access to grouped level mechanisms.
   /// \return immutable reference to the mechanism registry.
   const GameMechanismRegistry& getMechanismRegistry() const override;

   /// \brief returns all rooms parsed from the level.
   /// \return immutable reference to room list.
   const std::vector<std::shared_ptr<Room>>& getRooms() const override;

protected:
   /// \brief loads or regenerates physics paths for a collision tile layer and adds chains to box2d.
   /// \param layer tmx tile layer that defines physics collision tiles.
   /// \param tileSet tileset used for tile-to-collision conversion.
   /// \param basePath directory where generated physics files are read or written.
   void parsePhysicsTiles(
      const std::shared_ptr<TmxLayer>& layer,
      const std::shared_ptr<TmxTileSet>& tileSet,
      const std::filesystem::path& basePath
   );

   /// \brief converts tile-space paths to box2d loops and registers each as a world fixture.
   /// \param offsetX horizontal tile offset added to every path point.
   /// \param offsetY vertical tile offset added to every path point.
   /// \param paths square marcher paths to convert.
   /// \param behavior object type assigned to created fixtures.
   void addPathsToWorld(int32_t offsetX, int32_t offsetY, const std::vector<SquareMarcher::Path>& paths, ObjectType behavior);

   /// \brief creates a static box2d chain loop and attaches fixturenode metadata.
   /// \param chain loop vertices in box2d world coordinates.
   /// \param behavior object type stored in fixture user data.
   void addChainToWorld(const std::vector<b2Vec2>& chain, ObjectType behavior);

   /// \brief reads an obj mesh, converts faces to chain loops, and adds them as physics fixtures.
   /// \param layer tmx layer used for pixel offset and winding handling.
   /// \param behavior object type assigned to created fixtures.
   /// \param path path to the obj file containing optimized physics outlines.
   void parseObj(const std::shared_ptr<TmxLayer>& layer, ObjectType behavior, const std::filesystem::path& path);

   /// \brief loads tmx data, ambient occlusion data, and starts file watching for hot-reload detection.
   /// \return true when loading succeeds and required files are available.
   bool load();

   /// \brief parses tmx content, deserializes mechanisms, tile maps, rooms, lights, and physics layers.
   void loadTmx();

   /// \brief restores checkpoint spawn position and deserializes saved mechanism state.
   void loadSaveState();

   /// \brief initializes level.lua and binds mechanism lookup callbacks.
   void loadLevelScript();

   /// \brief generates unoptimized physics geometry, merges and optimizes paths, then loads them.
   /// \param layer tmx layer used to generate collision geometry.
   /// \param tileset tileset used by physics geometry extraction.
   /// \param base_path base directory for generated intermediate and output files.
   /// \param parse_data filenames and object type for the selected collision mode.
   /// \param path_solid_optimized output path for the optimized obj file.
   void regenerateLevelPaths(
      const std::shared_ptr<TmxLayer>& layer,
      const std::shared_ptr<TmxTileSet>& tileset,
      const std::filesystem::path& base_path,
      ParseData* parse_data,
      auto path_solid_optimized
   );

   /// \brief assigns overlapping room ids to mechanisms and lua enemies using their bounding boxes.
   void assignMechanismsToRooms();

   /// \brief saves a render texture to disk when screenshot capture is enabled.
   /// \param basename filename prefix used for generated screenshot files.
   /// \param texture render texture to save.
   void takeScreenshot(const std::string& basename, sf::RenderTexture& texture);

   /// \brief refreshes player light position and visibility state.
   void updatePlayerLight();

   /// \brief refreshes the current room from the player's current position.
   void updateRoom();

   /// \brief draws mechanisms from all groups that pass predicate at a specific z layer.
   /// \param color color render target.
   /// \param normal normal-map render target.
   /// \param z_index z layer to draw.
   /// \param predicate returns true for mechanisms that should be drawn.
   void drawMechanismsAtZ(
      sf::RenderTarget& color,
      sf::RenderTarget& normal,
      int32_t z_index,
      auto predicate,
      const sf::RenderStates& states = {}
   );

   /// \brief draws parallax tile maps at a specific z layer.
   /// \param target render target.
   /// \param z_index z layer to draw.
   void drawParallaxMaps(sf::RenderTarget& target, int32_t z_index);

   /// \brief draws all mechanisms flagged as post-lighting directly onto target after the lighting
   ///        pass so normal-map lighting does not render on top of them.
   /// \param target render target (the deferred composite texture).
   void drawPostLightingLayers(sf::RenderTarget& target);

   /// \brief draws all mechanisms flagged as overlay directly onto target after the post-lighting
   ///        pass so they appear on top of all other layers including post-lighting image layers.
   /// \param target render target (the deferred composite texture).
   void drawOverlayLayers(sf::RenderTarget& target);

   /// \brief draws tile maps, mechanisms, enemies, player, and image layers for a z-range.
   /// \param color color render target.
   /// \param normal normal render target.
   /// \param from first z layer to draw.
   /// \param to last z layer to draw.
   void drawLayers(sf::RenderTarget& color, sf::RenderTarget& normal, int32_t from, int32_t to);

   /// \brief renders the atmosphere tile map into the atmosphere shader render texture.
   void drawAtmosphereLayer();

   /// \brief draws glow-contributing elements into the blur target.
   /// \param target render target.
   void drawBlurLayer(sf::RenderTarget& target);

   /// \brief renders light sources into the lighting target.
   void drawLightMap();

   /// \brief draws the player sprite and normal map contribution.
   /// \param color color render target.
   /// \param normal normal render target.
   void drawPlayer(sf::RenderTarget& color, sf::RenderTarget& normal, const sf::RenderStates& states = {});

   /// \brief draws cached physics outline chains for debug visualization.
   /// \param target render target.
   void drawStaticChains(sf::RenderTarget& target);

   /// \brief draws debug overlays such as bodies, hitboxes, and room bounds when enabled.
   void drawDebugInformation();

   /// \brief draws z=24 tilemap geometry to the stencil buffer for light occlusion.
   /// \param target render target with active stencil context.
   void drawLightOccluders(sf::RenderTarget& target);

   /// \brief finalizes and displays intermediate level and normal render textures.
   void displayFinalTextures();

   /// \brief builds the blur texture used for additive glow composition.
   void drawGlowLayer();

   /// \brief composites the blurred glow texture back onto the main level target.
   void drawGlowSprite();

   std::vector<std::shared_ptr<Room>> _rooms;
   LevelScript _level_script;

   const RenderTargets& _render_targets;

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
   sfcompat::Shader _occluder_shader;  //!< alpha-test shader for light occluder stencil rendering
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

#ifdef DEVELOPMENT_MODE
   bool _mechanism_profiling_enabled{false};
#endif
};
