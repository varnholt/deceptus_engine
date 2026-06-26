#include "level.h"

// game
#include "framework/math/maptools.h"
#include "framework/pathmerger/pathmerger.h"
#include "framework/tmxparser/tmxelement.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxobjectgroup.h"
#include "framework/tmxparser/tmxparser.h"
#include "framework/tmxparser/tmxtileset.h"
#include "framework/tools/checksum.h"
#include "framework/tools/log.h"
#include "framework/tools/timer.h"
#include "game/animation/animationplayer.h"
#include "game/camera/camerapanorama.h"
#include "game/camera/cameraroomlock.h"
#include "game/camera/camerasystem.h"
#include "game/camera/camerazoom.h"
#include "game/config/gameconfiguration.h"
#include "game/config/tweaks.h"
#include "game/constants.h"
#include "game/debug/debugdraw.h"
#include "game/ingamemenu/ingamemenumap.h"
#include "game/io/gamedeserializedata.h"
#include "game/io/meshtools.h"
#include "game/level/fixturenode.h"
#include "game/level/leveldescription.h"
#include "game/level/levelfiles.h"
#include "game/level/luainterface.h"
#include "game/level/parsedata.h"
#include "game/level/roomupdater.h"
#include "game/level/tilemap.h"
#include "game/level/tilemapfactory.h"
#include "game/mechanisms/checkpoint.h"
#include "game/mechanisms/conveyorbelt.h"
#include "game/mechanisms/door.h"
#include "game/mechanisms/extra.h"
#include "game/mechanisms/gamemechanismdeserializer.h"
#include "game/mechanisms/gamemechanismdeserializerconstants.h"
#include "game/mechanisms/lever.h"
#include "game/physics/chainshapeanalyzer.h"
#include "game/physics/gamecontactlistener.h"
#include "game/physics/physicsconfiguration.h"
#include "game/physics/squaremarcher.h"
#include "game/player/player.h"
#include "game/player/playerfirefly.h"
#include "game/player/playerregistry.h"
#include "game/player/playerstencil.h"
#include "game/state/displaymode.h"
#include "game/state/savestate.h"
#include "game/weapons/gun.h"

// sfml
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

#ifdef DEVELOPMENT_MODE
#include "game/debug/mechanismsample.h"
#endif

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <execution>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ranges>
#include <regex>
#include <span>
#include <sstream>
#include <string>
#include <thread>

// #define MECHANISM_TIMING_ENABLED 1

#ifdef DEVELOPMENT_MODE
namespace
{
struct MechanismTiming
{
   using HighResDuration = std::chrono::high_resolution_clock::duration;

   HighResDuration update_duration;
   HighResDuration draw_duration;
   int32_t update_count{0};
   int32_t draw_count{0};

   void addUpdateTime(HighResDuration duration)
   {
      update_duration += duration;
      update_count++;
   }

   void addDrawTime(HighResDuration duration)
   {
      draw_duration += duration;
      draw_count++;
   }

   float getAverageUpdateMs() const
   {
      return (update_count > 0) ? std::chrono::duration<float, std::milli>(update_duration).count() / static_cast<float>(update_count)
                                : 0.0f;
   }

   float getAverageDrawMs() const
   {
      return (draw_count > 0) ? std::chrono::duration<float, std::milli>(draw_duration).count() / static_cast<float>(draw_count) : 0.0f;
   }
};

std::unordered_map<std::string, MechanismTiming> timing_data;

}  // namespace
#endif

namespace
{

bool checkUpdateMechanism(const auto& player_chunk, const auto& mechanism)
{
   auto update_mechanism = true;
   if (mechanism->hasChunks())
   {
      const auto& chunks = mechanism->getChunks();
      update_mechanism = std::any_of(
         chunks.cbegin(),
         chunks.cend(),
         [player_chunk](const Chunk& other)
         { return (abs(player_chunk._x - other._x) < CHUNK_ALLOWED_DELTA_X) && (abs(player_chunk._y - other._y) < CHUNK_ALLOWED_DELTA_Y); }
      );
   }

   return update_mechanism;
};

}  // namespace

std::string Level::getDescriptionFilename() const
{
   return _description_filename;
}

void Level::setDescriptionFilename(const std::string& description_filename)
{
   _description_filename = description_filename;
}

const Atmosphere& Level::getAtmosphere() const
{
   return _atmosphere;
}

Level::Level(const RenderTargets& render_targets) : GameNode(nullptr), _render_targets(render_targets)
{
   setClassName(typeid(Level).name());

   _ambient_occlusion = std::make_unique<AmbientOcclusion>();
   _volume_updater = std::make_unique<VolumeUpdater>();

   // create shaders (render textures are owned by Game)
   _atmosphere_shader = std::make_unique<AtmosphereShader>();
   _blur_shader = std::make_unique<BlurShader>();
   _gamma_shader = std::make_unique<GammaShader>();

   // load alpha-test shader for occluder stencil rendering
#ifndef __EMSCRIPTEN__
   if (!_occluder_shader.loadFromFile("data/shaders/stencil_write.vert", "data/shaders/stencil_write.frag"))
   {
      Log::Warning() << "failed to load occluder stencil shader";
   }
   _occluder_shader.setUniform("u_texture_sampler", sf::Shader::CurrentTexture);
   _occluder_shader.setUniform("u_alpha_threshold", 0.01f);
#endif

   // init world for this level
   const b2Vec2 gravity(0.f, PhysicsConfiguration::getInstance()._gravity);

   LuaInterface::instance().reset();

   // clear those here so the world destructor doesn't double-delete them
   Projectile::clear();

   _world = std::make_shared<b2World>(gravity);

   GameContactListener::getInstance().reset();
   _world->SetContactListener(&GameContactListener::getInstance());

   _light_system = std::make_shared<LightSystem>();

   // set up occluder callback for light occlusion (z=24 "level" layer)
   _light_system->setOccluderCallback([this](sf::RenderTarget& target) { drawLightOccluders(target); });

   // add raycast light for player
   nlohmann::json player_light_config;
   std::ifstream("data/config/player_light.json") >> player_light_config;
   _player_light =
      LightSystem::createLightInstance(std::static_pointer_cast<Player>(PlayerRegistry::getFirst()).get(), player_light_config);
   _light_system->_lights.push_back(_player_light);
}

Level::~Level()
{
   Log::Info() << "deleting current level";

   // stop active timers because their callbacks being called after destruction of the level/world can be nasty
   for (const auto& enemy : LuaInterface::instance().getObjectList())
   {
      Timer::removeByCaller(enemy);
   }

   _file_watcher_thread_active = false;
   if (_file_watcher_thread.joinable())
   {
      _file_watcher_thread.join();
   }
}

// assign room identifiers to mechanism
// for now it's safe to assume that a mechanism always stays in the same room
void Level::assignMechanismsToRooms()
{
   auto update_room = [this](const std::shared_ptr<GameMechanism>& mechanism)
   {
      if (mechanism->getBoundingBoxPx().has_value())
      {
         auto rooms = Room::findAll(mechanism->getBoundingBoxPx().value(), _rooms);
         for (const auto& room : rooms)
         {
            mechanism->addRoomId(room->_id);
         }
      }
   };

   for (auto& mechanism_vector : _mechanism_registry.getList())
   {
      for (auto& mechanism : *mechanism_vector)
      {
         update_room(mechanism);
      }
   }

   for (const auto& enemy : LuaInterface::instance().getObjectList())
   {
      update_room(enemy);
   }
}

void Level::loadTmx()
{
   static const std::string parallax_identifier = "parallax_";
   const auto path = std::filesystem::path(_description->_filename).parent_path();

#ifdef DEVELOPMENT_MODE
   // checksum checking won't be needed in production
   const auto checksum_old = Checksum::readChecksum(_description->_filename + ".crc");
   const auto checksum_new = Checksum::calcChecksum(_description->_filename);
   const auto checksum_mismatch = checksum_old != checksum_new;

   if (checksum_mismatch || _loading_mode == LoadingMode::Clean)
   {
      LevelFiles::clean(*_description);
      Checksum::writeChecksum(_description->_filename + ".crc", checksum_new);
   }
#endif

   sf::Clock elapsed;

   // parse tmx
   TmxParser tmx_parser;
   Log::Info() << "parsing tmx: " << _description->_filename;
   tmx_parser.parse(_description->_filename);
   Log::Info() << "parsing tmx, done within " << elapsed.getElapsedTime().asSeconds() << "s";

   Log::info("loading tmx... ");
   elapsed.restart();

   GameDeserializeData data;
   data._world = _world;
   data._base_path = path;

   GameMechanismDeserializer::deserialize(tmx_parser, this, data, _mechanism_registry.getMap());

   // preload mechanism data in parallel
   for (auto& [vec_key, vec_values] : _mechanism_registry.getMap())
   {
#if defined(__APPLE__) || defined(__EMSCRIPTEN__)
      std::for_each(vec_values->begin(), vec_values->end(), [](auto& val) { val->preload(); });
   }

   // process everything that's not considered a mechanism
   const auto& tmx_elements = tmx_parser.getElements();

   // collect all layer data first for parallel loading
   struct LayerLoadData
   {
      std::shared_ptr<TmxLayer> layer;
      std::shared_ptr<TmxTileSet> tileset;
      std::shared_ptr<TileMap> tile_map;
      bool push_tile_map = true;
   };
   std::vector<LayerLoadData> layer_load_data;

   for (const auto& element : tmx_elements)
   {
      data._tmx_layer = nullptr;
      data._tmx_tileset = nullptr;
      data._tmx_object = nullptr;
      data._tmx_object_group = nullptr;

      // parse sprites
      if (element->_type == TmxElement::Type::TypeLayer)
      {
         auto layer = std::dynamic_pointer_cast<TmxLayer>(element);
         if (GameMechanismDeserializer::isLayerNameReserved(layer->_name))
         {
            continue;
         }

         const auto tileset = tmx_parser.getTileSet(layer);

         data._tmx_layer = layer;
         data._tmx_tileset = tileset;

         auto tile_map = TileMapFactory::makeTileMap(layer);
         layer_load_data.push_back({layer, tileset, tile_map, true});
      }

      // parse objects
      else if (element->_type == TmxElement::Type::TypeObjectGroup)
      {
         const auto object_group = std::dynamic_pointer_cast<TmxObjectGroup>(element);

         for (const auto& object : object_group->_objects)
         {
            const auto tmx_object = object.second;
            data._tmx_object = tmx_object;
            data._tmx_object_group = object_group;

            if (object_group->_name == "enemies")
            {
               TmxEnemy enemy;
               enemy.parse(tmx_object);
               _enemy_data_from_tmx_layer[enemy._id] = enemy;
            }
            else if (object_group->_name == "rooms")
            {
               Room::deserialize(this, data, _rooms);
            }
            else if (object_group->_name == "lights")
            {
               const auto light = LightSystem::createLightInstance(this, data);
               _light_system->_lights.push_back(light);
            }
         }
      }

      // parse images
      else if (element->_type == TmxElement::Type::TypeImageLayer)
      {
         const auto image = ImageLayer::deserialize(element, path);
         _mechanism_registry.addImageLayer(image);
      }
   }

   // kick off background disk loads for all image layer textures so they are in RAM
   // by the time the drain loop runs at the end of this function
   for (auto& image_layer : _mechanism_registry.getImageLayers())
   {
      image_layer->preload();
   }

   // load tilemaps in parallel
#if defined(__APPLE__) || defined(__EMSCRIPTEN__)
   std::for_each(
      layer_load_data.begin(),
      layer_load_data.end(),
      [&path](auto& layer_data) { layer_data.tile_map->load(layer_data.layer, layer_data.tileset, path); }
   );
#else
   std::for_each(
      std::execution::par,
      layer_load_data.begin(),
      layer_load_data.end(),
      [&path](auto& layer_data) { layer_data.tile_map->load(layer_data.layer, layer_data.tileset, path); }
   );
#endif

   // process loaded tilemaps sequentially
   for (auto& layer_data : layer_load_data)
   {
      const auto& layer = layer_data.layer;
      const auto& tileset = layer_data.tileset;
      const auto& tile_map = layer_data.tile_map;

      if (layer->_name == "atmosphere")
      {
         _atmosphere._tile_map = tile_map;
         _atmosphere.parse(layer, tileset);
      }
      else if (layer->_name.compare(0, parallax_identifier.length(), parallax_identifier) == 0)
      {
         auto parallax_layer = ParallaxLayer::deserialize(layer, tile_map);
         _parallax_layers.push_back(std::move(parallax_layer));
         layer_data.push_tile_map = false;
      }
      else if (layer->_name == "level" || layer->_name == "level_solid_onesided" || layer->_name == "level_deadly")
      {
         parsePhysicsTiles(layer, tileset, path);
      }

      if (layer_data.push_tile_map)
      {
         _tile_maps.push_back(tile_map);
      }
   }

   TileMapFactory::merge(_tile_maps);
   Room::mergeEnterAreas(_rooms);

   if (!_atmosphere._tile_map)
   {
      Log::Error() << "fatal: no physics layer (called 'physics') found!";
   }

   // upload all pending image layer textures to GPU before gameplay begins —
   // background threads have been running disk I/O since the preload call above,
   // so this loop typically only waits for GPU transfers, not disk reads
   {
      const auto image_layers = _mechanism_registry.getImageLayers();
      bool any_pending = true;
      while (any_pending)
      {
         any_pending = false;
         for (auto& image_layer : image_layers)
         {
            if (image_layer->drainTextures())
            {
               any_pending = true;
            }
         }
         if (any_pending)
         {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
         }
      }
   }

   Log::Info() << "loading tmx, done within " << elapsed.getElapsedTime().asSeconds() << "s";
}

BoomEffect& Level::getBoomEffect()
{
   return _boom_effect;
}

bool Level::load()
{
   const auto level_json_path = std::filesystem::path(_description->_filename);
   setObjectId(_description->_filename);

   if (!std::filesystem::exists(level_json_path))
   {
      Log::Error() << "path " << level_json_path << " does not exist";
      return false;
   }

   loadTmx();

   _ambient_occlusion->load(level_json_path.parent_path(), std::filesystem::path(_description->_filename).stem().string());

   Log::Info() << "level loading complete";

   // set up file watcher
   _file_watcher_thread = std::thread(
      [this]()
      {
         auto first_modified_time = std::filesystem::last_write_time(_description->_filename);

         while (_file_watcher_thread_active)
         {
            const auto current_modified_time = std::filesystem::last_write_time(_description->_filename);
            if (current_modified_time != first_modified_time)
            {
               Log::Info() << "level was modified, marking as dirty";
               first_modified_time = current_modified_time;
               _dirty = true;
            }

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1s);
         }
      }
   );

   return true;
}

void Level::loadStartPosition()
{
   _start_position.x = static_cast<float_t>(_description->_start_position.at(0) * PIXELS_PER_TILE + PLAYER_ACTUAL_WIDTH / 2);
   _start_position.y = static_cast<float_t>(_description->_start_position.at(1) * PIXELS_PER_TILE + DIFF_PLAYER_TILE_TO_PHYSICS);
}

void Level::loadLevelScript()
{
   const auto path = std::filesystem::path(_description->_filename).parent_path();
   _level_script.setup(path / "level.lua");
   _level_script.setSearchMechanismCallback([this](const std::string& regexPattern, const std::optional<std::string>& group)
                                            { return _mechanism_registry.searchMechanisms(regexPattern, group); });

   _level_script.createExtraCallbacks(_mechanism_registry.getExtras());
}

void Level::initialize()
{
   createViews();

   _description = LevelDescription::load(_description_filename);

   if (!_description)
   {
      Log::Error() << "level configuration is bad, could not load " << _description_filename;
      return;
   }

   if (!load())
   {
      Log::Error() << "level loading failed, filename: " << _description_filename;
      return;
   }

   // initialize shaders with render targets from Game
   _atmosphere_shader->initialize(_render_targets.atmosphere);
   _blur_shader->initialize(_render_targets.blur, _render_targets.blur_scaled);
   _gamma_shader->initialize();
#endif

   loadStartPosition();

   loadSaveState();
   spawnEnemies();

   auto player_firefly = std::make_shared<PlayerFirefly>(this);
   _mechanism_registry.getMap()[std::string{layer_name_fireflies}]->push_back(player_firefly);

   assignMechanismsToRooms();
   _volume_updater->setMechanisms(_mechanism_registry.getList());

   loadLevelScript();

   // dump();
}

void Level::loadSaveState()
{
   const auto& save_state = SaveState::getCurrent();
   auto checkpoint_index = save_state._checkpoint;
   auto checkpoint = Checkpoint::getCheckpoint(checkpoint_index, _mechanism_registry.getCheckpoints());

   if (checkpoint)
   {
      auto pos = checkpoint->spawnPoint();
      _start_position.x = static_cast<float>(pos.x);
      _start_position.y = static_cast<float>(pos.y);
      Log::Info() << "move to checkpoint: " << checkpoint_index;
   }
   else
   {
      Log::Error() << "level doesn't have a start check point set up";
   }

   if (save_state._level_state.is_null())
   {
      return;
   }

   const auto& level_json = save_state._level_state[_description->_filename];
   if (level_json.is_null())
   {
      return;
   }

   const auto& mechanism_map = _mechanism_registry.getMap();
   for (auto& [mechanism_key, mechanism_values] : level_json.items())
   {
      const auto& mechanism_it = mechanism_map.find(mechanism_key);
      if (mechanism_it == mechanism_map.end())
      {
         continue;
      }

      const auto& mechanism_vector = mechanism_it->second;
      for (auto& [object_key, object_value] : mechanism_values.items())
      {
         auto result = std::find_if(
            mechanism_vector->begin(),
            mechanism_vector->end(),
            [object_key](const auto& object)
            {
               auto* game_node = dynamic_cast<GameNode*>(object.get());
               return (game_node && game_node->getObjectId() == object_key);
            }
         );

         if (result != mechanism_vector->end())
         {
            (*result)->deserializeState(object_value);
         }
      }
   }
}

void Level::saveState()
{
   auto& j = SaveState::getCurrent()._level_state;

   nlohmann::json mechanisms_json;

   // serialize the states of all mechanisms
   const auto& mechanism_map = _mechanism_registry.getMap();
   for (auto& [key, mechanisms] : mechanism_map)
   {
      nlohmann::json mechanism_json;
      for (auto& mechanism : *mechanisms)
      {
         mechanism->serializeState(mechanism_json);
      }

      if (!mechanism_json.empty())
      {
         mechanisms_json[key] = mechanism_json;
      }
   }

   j[_description->_filename] = mechanisms_json;
}

void Level::reset()
{
   _mechanism_registry.resetDoors();
}

void Level::spawnEnemies()
{
   // those enemies that have a lua script associated inside the tmx layer don't need
   // additional information from json, those can just be spawned.
   // this should probably be the future and only approach how to handle enemy spawning.
   for (auto& it : _enemy_data_from_tmx_layer)
   {
      const auto script = it.second.findProperty("script");

      if (!script.has_value())
      {
         Log::Error() << it.first << " does not have any script definition";
         continue;
      }

      auto lua_node = LuaInterface::instance().addObject(this, std::string("data/scripts/enemies/") + script.value()._value);

      EnemyDescription json_description;
      json_description._position_in_tiles = false;
      json_description._start_position.push_back(it.second._pixel_position.x);
      json_description._start_position.push_back(it.second._pixel_position.y);
      json_description._id = it.second._id;
      json_description._name = it.second._name;

      const auto& generate_path_property = it.second.findProperty("generate_path");
      const auto& inverse_path_property = it.second.findProperty("inverse_path");
      if (generate_path_property.has_value() && generate_path_property.value()._value == "1")
      {
         it.second._inverse_path = (inverse_path_property.has_value() && inverse_path_property.value()._value == "1");
         it.second.addPaths(_world_chains);
      }

      if (!it.second._pixel_path.empty())
      {
         json_description._path = it.second._pixel_path;
      }

      // z index
      const auto& z_index_property = it.second.findProperty("z");
      if (z_index_property.has_value())
      {
         lua_node->setZ(std::stoi(z_index_property.value()._value));
      }

      // merge properties from tmx with those loaded from json
      for (auto& property : it.second._properties)
      {
         json_description._properties.push_back(property);
      }

      // initialize lua node and store enemy
      lua_node->_enemy_description = json_description;
      lua_node->initialize();
   }
}

void Level::drawStaticChains(sf::RenderTarget& target)
{
   for (auto& path : _atmosphere._outlines)
   {
      target.draw(std::span<const sf::Vertex>{&path.at(0), path.size()}, sf::PrimitiveType::LineStrip);
   }
}

const std::shared_ptr<sf::View>& Level::getLevelView() const
{
   return _level_view;
}

void Level::createViews()
{
   auto& game_config = GameConfiguration::getInstance();

   // the view dimensions never change
   _view_width = static_cast<float>(game_config._view_width);
   _view_height = static_cast<float>(game_config._view_height);

   _level_view.reset();
#ifndef __EMSCRIPTEN__
   _level_view = std::make_shared<sf::View>(sf::FloatRect({0.0f, 0.0f}, {_view_width, _view_height}));
   _level_view->setViewport(sf::FloatRect({0.0f, 0.0f}, {1.0f, 1.0f}));

   for (auto& parallax_layer : _parallax_layers)
   {
      parallax_layer->resetView(_view_width, _view_height);
   }

   for (auto& image_layer : _mechanism_registry.getImageLayers())
   {
      image_layer->resetView(_view_width, _view_height);
   }
}

void Level::updateViews()
{
   // this should really just fetch the camera position and the camera panorama vectors and
   // update the views with them; no camera or camera panorama correction must be done here
   const auto& look_vector = CameraPanorama::getInstance().getLookVector();
   const auto& camera_system = CameraSystem::getInstance();
   const auto level_view_x = camera_system.getX() + look_vector.x;
   const auto level_view_y = camera_system.getY() + look_vector.y;

   auto& zoom = CameraZoom::getInstance();
   auto view_rect = sf::FloatRect{{level_view_x, level_view_y}, {_view_width, _view_height}};
   zoom.adjust(view_rect);

   CameraRoomLock::setViewRect(view_rect);

   _level_view = std::make_shared<sf::View>(view_rect);

   for (const auto& parallax : _parallax_layers)
   {
      parallax->updateView(view_rect.position.x, view_rect.position.y, view_rect.size.x, view_rect.size.y);
   }

   for (const auto& image_layer : _mechanism_registry.getImageLayers())
   {
      image_layer->updateView(view_rect.position.x, view_rect.position.y, view_rect.size.x, view_rect.size.y);
   }
}

void Level::updateMechanismVolumes()
{
   if (!_volume_updater)
   {
      return;
   }

   _volume_updater->setPlayerPosition(PlayerRegistry::getFirst()->getPixelPositionFloat());
}

void Level::updateRoom()
{
   RoomUpdater::setCurrent(Room::find(PlayerRegistry::getFirst()->getPixelPositionFloat(), _rooms));
}

void Level::syncRoom()
{
   RoomUpdater::setCurrent(Room::find(PlayerRegistry::getFirst()->getPixelPositionFloat(), _rooms));
   CameraRoomLock::setRoom(RoomUpdater::getCurrent());
}

void Level::updateCameraSystem(const sf::Time& dt)
{
   auto& camera_system = CameraSystem::getInstance();

   // update room
   const auto room_previous = RoomUpdater::getCurrent();
   updateRoom();
   const auto room_current = RoomUpdater::getCurrent();

   // room changed
   if (room_previous != room_current)
   {
      std::string room_id = "undefined";
      std::string enter_area_name = "undefined";
      if (room_current)
      {
         room_id = room_current->getObjectId();
         const auto entered_area = room_current->enteredArea(PlayerRegistry::getFirst()->getPixelPositionFloat());
         if (entered_area.has_value())
         {
            enter_area_name = entered_area.value()._name;
         }
      }

      Log::Info() << "player moved to room: " << room_id << " on side " << enter_area_name;

      // will update the current room in both cases, either after the camera lock delay or instantly
      if (room_current && room_current->_camera_lock_delay.has_value())
      {
         room_current->lockCamera();
      }
      else
      {
         CameraRoomLock::setRoom(room_current);
      }

      // trigger transition effect
      // when level has been loaded, room changes certainly do not require a transition
      if (RoomUpdater::isSynced() && room_current)
      {
         room_current->startTransition();
      }
   }

   // update camera system
   if (!room_current || !room_current->_camera_locked)
   {
      camera_system.update(dt, _view_width, _view_height);
   }

   RoomUpdater::setSynced(true);

   CameraZoom::getInstance().update(dt);
}

void Level::zoomIn()
{
   auto& zoom = CameraZoom::getInstance();
   zoom.setZoomFactor(zoom.getZoomFactor() * 0.95);
}

void Level::zoomOut()
{
   auto& zoom = CameraZoom::getInstance();
   zoom.setZoomFactor(zoom.getZoomFactor() * 1.05);
}

void Level::zoomBy(float delta)
{
   auto& zoom = CameraZoom::getInstance();
   zoom.setZoomFactor(std::clamp(zoom.getZoomFactor() + delta * 0.2f, 0.1f, 20.0f));
}

void Level::zoomReset()
{
   auto& zoom = CameraZoom::getInstance();
   zoom.setZoomFactor(1.0f);
}

void Level::drawLightMap()
{
   _render_targets.lighting->clear();
   _render_targets.lighting->setView(*_level_view);
#endif
   _render_targets.lighting2->clear();
#ifndef __EMSCRIPTEN__
   _render_targets.lighting2->setView(*_level_view);
#endif
   _light_system->draw(*_render_targets.lighting, *_render_targets.lighting2, {});
   _render_targets.lighting->display();
   _render_targets.lighting2->display();

   //   static int32_t light_map_save_counter = 0;
   //   light_map_save_counter++;
   //   if (light_map_save_counter % 60 == 0)
   //   {
   //      _render_texture_lighting->getTexture().copyToImage().saveToFile("light_map.png");
   //   }
}

void Level::drawParallaxMaps(sf::RenderTarget& target, int32_t z_index)
{
   for (const auto& parallax : _parallax_layers)
   {
      if (parallax->_z_index == z_index)
      {
#ifndef __EMSCRIPTEN__
         target.setView(parallax->_view);
#endif
         target.draw(*parallax->_tile_map);
      }
   }

   // restore level view
#ifndef __EMSCRIPTEN__
   target.setView(*_level_view);
#endif
}

void Level::drawMechanismsAtZ(sf::RenderTarget& color, sf::RenderTarget& normal, int32_t z_index, auto predicate)
{
   for (auto* mechanism_vector : _mechanism_registry.getList())
   {
      for (const auto& mechanism : *mechanism_vector)
      {
         if (mechanism->getZ() == z_index && predicate(mechanism))
         {
#ifdef DEVELOPMENT_MODE
            if (_mechanism_profiling_enabled)
            {
               const auto mechanism_name = std::string{mechanism->objectName()};
               const auto time_start = std::chrono::high_resolution_clock::now();
               mechanism->draw(color, normal);
               timing_data[mechanism_name].addDrawTime(std::chrono::high_resolution_clock::now() - time_start);
            }
            else
            {
               mechanism->draw(color, normal);
            }
#else
               mechanism->draw(color, normal);
#endif
         }
      }
   }
}

void Level::drawPostLightingLayers(sf::RenderTarget& target)
{
#ifndef __EMSCRIPTEN__
   const auto previous_view = target.getView();
   target.setView(*_level_view);
#endif
   const auto& player_chunk = PlayerRegistry::getFirst()->getChunk();

   for (auto z_index = static_cast<int32_t>(ZDepth::BackgroundMin); z_index <= static_cast<int32_t>(ZDepth::ForegroundMax); z_index++)
   {
      for (const auto& tile_map : _tile_maps)
      {
         if (tile_map->getZ() == z_index && tile_map->isPostLighting())
         {
            tile_map->draw(target, target, {});
         }
      }

      drawMechanismsAtZ(
         target,
         target,
         z_index,
         [&player_chunk](const auto& mechanism) { return mechanism->isPostLighting() && checkUpdateMechanism(player_chunk, mechanism); }
      );

      for (auto& layer : _mechanism_registry.getImageLayers())
      {
         if (layer->getZ() == z_index && layer->isPostLighting())
         {
            layer->draw(target, target);
         }
      }
   }

#ifndef __EMSCRIPTEN__
   target.setView(previous_view);
#endif
}

void Level::drawOverlayLayers(sf::RenderTarget& target)
{
#ifndef __EMSCRIPTEN__
   const auto previous_view = target.getView();
   target.setView(*_level_view);
#endif
   const auto& player_chunk = PlayerRegistry::getFirst()->getChunk();

   for (auto z_index = static_cast<int32_t>(ZDepth::BackgroundMin); z_index <= static_cast<int32_t>(ZDepth::ForegroundMax); z_index++)
   {
      drawMechanismsAtZ(
         target,
         target,
         z_index,
         [&player_chunk](const auto& mechanism) { return mechanism->isOverlay() && checkUpdateMechanism(player_chunk, mechanism); }
      );
   }

#ifndef __EMSCRIPTEN__
   target.setView(previous_view);
#endif
}

void Level::drawPlayer(sf::RenderTarget& color, sf::RenderTarget& normal)
{
   std::static_pointer_cast<Player>(PlayerRegistry::getFirst())->draw(color, normal);
}

void Level::drawLayers(sf::RenderTarget& target, sf::RenderTarget& normal, int32_t from, int32_t to)
{
   const auto& player_chunk = PlayerRegistry::getFirst()->getChunk();

#ifndef __EMSCRIPTEN__
   target.setView(*_level_view);
   normal.setView(*_level_view);
#endif

   for (auto z_index = from; z_index <= to; z_index++)
   {
      PlayerStencil::draw(target, z_index);
      drawParallaxMaps(target, z_index);

      // draw all tile maps
      for (const auto& tile_map : _tile_maps)
      {
         if (tile_map->getZ() == z_index && !tile_map->isPostLighting())
         {
            tile_map->draw(target, normal, {});
         }
      }

      // draw mechanisms
      drawMechanismsAtZ(
         target,
         normal,
         z_index,
         [&player_chunk](const auto& mechanism)
         { return !mechanism->isPostLighting() && !mechanism->isOverlay() && checkUpdateMechanism(player_chunk, mechanism); }
      );

      // ambient occlusion
      if (z_index == _ambient_occlusion->getZ())
      {
         _ambient_occlusion->draw(target);
      }

      // draw enemies
      for (auto& enemy : LuaInterface::instance().getObjectList())
      {
         if (enemy->getZ() == z_index)
         {
            if (checkUpdateMechanism(player_chunk, enemy))
            {
               enemy->draw(target, normal);
            }
         }
      }

      // draw player
      if (z_index == static_cast<int32_t>(ZDepth::Player))
      {
         drawPlayer(target, normal);
      }

      // draw image layers; post-lighting layers are drawn after the lighting pass
      for (auto& layer : _mechanism_registry.getImageLayers())
      {
         if (layer->getZ() == z_index)
         {
            if (layer->isPostLighting())
            {
               continue;
            }
            layer->draw(target, normal);
         }
      }
   }
}

void Level::drawAtmosphereLayer()
{
#ifndef __EMSCRIPTEN__
   if (!_atmosphere._tile_map)
   {
      return;
   }

   _atmosphere_shader->getRenderTexture()->clear();
   _atmosphere._tile_map->setVisible(true);
   _atmosphere_shader->getRenderTexture()->draw(*_atmosphere._tile_map);
   _atmosphere._tile_map->setVisible(false);
   _atmosphere_shader->getRenderTexture()->display();
#endif
}

void Level::drawBlurLayer(sf::RenderTarget& target)
{
#ifndef __EMSCRIPTEN__
   target.setView(*_level_view);
#endif

   // draw elements that are supposed to glow / to be blurred here

#ifdef GLOW_ENABLED
   // lasers have been removed here because dstar added the glow to the spriteset

   const auto pPos = PlayerRegistry::getFirst()->getPixelPositionf();

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

bool Level::isPhysicsPathClear(const sf::Vector2i& a_tl, const sf::Vector2i& b_tl) const
{
   if (a_tl.x < 0 || a_tl.y < 0 || b_tl.x < 0 || b_tl.y < 0 || a_tl.x > _physics._grid_width || b_tl.x > _physics._grid_width ||
       a_tl.y > _physics._grid_height || b_tl.y > _physics._grid_height)
   {
      return false;
   }

   auto blocks = [this](uint32_t x, uint32_t y) -> bool { return _physics._physics_map[(_physics._grid_width * y) + x] == 1; };

   return MapTools::lineCollide(a_tl.x, a_tl.y, b_tl.x, b_tl.y, blocks);
}

void Level::takeScreenshot(const std::string& basename, sf::RenderTexture& texture)
{
   if (!_screenshot)
   {
      return;
   }

   std::ostringstream ss;
   ss << basename << "_" << std::setw(2) << std::setfill('0') << _screenshot_counters[basename] << ".png";
   _screenshot_counters[basename]++;
   texture.getTexture().copyToImage().saveToFile(ss.str());
}

void Level::drawDebugInformation()
{
   if (DisplayMode::getInstance().isSet(Display::Debug))
   {
      auto& texture = *_render_targets.level.get();
      drawStaticChains(texture);
      DebugDraw::debugBodies(texture, this);
      DebugDraw::drawRect(texture, PlayerRegistry::getFirst()->getPixelRectFloat());
      DebugDraw::debugHitboxes(texture);

      for (const auto& room : _rooms)
      {
         for (const auto& sub_room : room->_sub_rooms)
         {
            DebugDraw::drawRect(texture, sub_room._rect, sf::Color::Yellow);
         }
      }
   }
}

void Level::drawLightOccluders(sf::RenderTarget& target)
{
   // draw all tilemaps at z=24 into the stencil buffer only.
   // stencilOnly=true suppresses color output (replaces the old zero/zero blend mode).
   // the fragment shader's discard still runs so transparent tile regions don't occlude.
#ifndef __EMSCRIPTEN__
   target.setView(*_level_view);
#endif

   sf::RenderStates states;
#ifndef __EMSCRIPTEN__
   states.shader = &_occluder_shader;
#endif
   states.stencilMode = sf::StencilMode{
      .stencilComparison = sf::StencilComparison::Always,
      .stencilUpdateOperation = sf::StencilUpdateOperation::Replace,
      .stencilOnly = true,
      .stencilReference = sf::StencilValue{1u},
      .stencilMask = sf::StencilValue{~0u}
   };

   for (const auto& tile_map : _tile_maps)
   {
      if (tile_map->getZ() == 24 && tile_map->isVisible())
      {
         tile_map->draw(target, states);
      }
   }
}

void Level::displayFinalTextures()
{
   // display the whole texture
#ifndef __EMSCRIPTEN__
   {
      sf::View view(sf::FloatRect(
         {0.0f, 0.0f}, {static_cast<float>(_render_targets.level->getSize().x), static_cast<float>(_render_targets.level->getSize().y)}
      ));
      view.setViewport(sf::FloatRect({0.0f, 0.0f}, {1.0f, 1.0f}));
      _render_targets.level->setView(view);
      _render_targets.normal->setView(view);
   }
#endif
   _render_targets.level->display();
   _render_targets.normal->display();
}

void Level::drawGlowLayer()
{
#ifdef GLOW_ENABLED
   _blur_shader->clearTexture();
   drawBlurLayer(*_blur_shader->getRenderTexture().get());
   _blur_shader->getRenderTexture()->display();
   takeScreenshot("screenshot_blur", *_blur_shader->getRenderTexture().get());
#endif
}

void Level::drawGlowSprite()
{
#ifdef GLOW_ENABLED
   const sf::Texture& blur_texture = _blur_shader->getRenderTexture()->getTexture();
   sf::Sprite blur_sprite;
   const auto down_scale_x =
      _blur_shader->getRenderTextureScaled()->getSize().x / static_cast<float>(_blur_shader->getRenderTexture()->getSize().x);
   const auto down_scale_y =
      _blur_shader->getRenderTextureScaled()->getSize().y / static_cast<float>(_blur_shader->getRenderTexture()->getSize().y);
   blur_sprite.scale = {down_scale_x, down_scale_y};

   sf::RenderStates states_shader;
   states_shader.texture = &blur_texture;
   _blur_shader->update();
   states_shader.shader = &_blur_shader->getShader();
   _blur_shader->getRenderTextureScaled()->draw(blur_sprite, states_shader);

   const sf::Texture& blur_scale_texture = _blur_shader->getRenderTextureScaled()->getTexture();
   sf::Sprite blur_scale_sprite;
   blur_scale_sprite.scale = {1.0f / down_scale_x, 1.0f / down_scale_y};
   blur_scale_sprite.textureRect = sf::IntRect(
      0,
      static_cast<int32_t>(blur_scale_texture.getSize().y),
      static_cast<int32_t>(blur_scale_texture.getSize().x),
      -static_cast<int32_t>(blur_scale_texture.getSize().y)
   );

   sf::RenderStates states_add;
   states_add.blendMode = sf::BlendAdd;
   states_add.texture = &blur_scale_texture;
   _render_targets.level->draw(blur_scale_sprite, states_add);
#endif
}

const std::vector<std::shared_ptr<Room>>& Level::getRooms() const
{
   return _rooms;
}

const GameMechanismRegistry& Level::getMechanismRegistry() const
{
   return _mechanism_registry;
}

void Level::setLoadingMode(LoadingMode loading_mode)
{
   _loading_mode = loading_mode;
}

bool Level::isDirty() const
{
   return _dirty;
}

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
//    03) draw level background                                   -> level texture
//        - layers z=0..15
//    04) draw level background normals                           -> normal tmp texture
//        - layers z=0..15
//    05) draw level background with atmosphere shader on         -> background texture
//    06) draw level background normals with atmosphere shader on -> normal texture
//    07) draw level foreground                                   -> level texture
//        - layers z=16..50
//        - additive lights
//        - mechanisms
//        - ambient occlusion
//        - images with varying blend modes
//        - player
//    08) draw raycast lights                                     -> level texture
//    09) draw projectiles                                        -> level texture
//    10) flash and bounce -> move level texture
//    11) draw level texture with gamma shader enabled            -> straight to window
//    12) draw level map (if enabled)                             -> straight to window
//
void Level::draw(const std::shared_ptr<sf::RenderTexture>& window, bool screenshot)
{
   _screenshot = screenshot;

   // render atmosphere to atmosphere texture, that texture is used in the shader only
   drawAtmosphereLayer();
#ifndef __EMSCRIPTEN__
   takeScreenshot("texture_atmosphere", *_atmosphere_shader->getRenderTexture().get());
#endif

   // render glowing elements
   drawGlowLayer();

   // render layers affected by the atmosphere
   _render_targets.level->clear();
   _render_targets.level_background->clear();
   _render_targets.normal_tmp->clear();
   _render_targets.normal->clear();

   drawLayers(
      *_render_targets.level_background.get(),
      *_render_targets.normal_tmp.get(),
      static_cast<int32_t>(ZDepth::BackgroundMin),
      static_cast<int32_t>(ZDepth::BackgroundMax)
   );

   _render_targets.level_background->display();
   takeScreenshot("texture_level_background", *_render_targets.level_background.get());

   _render_targets.normal_tmp->display();
   takeScreenshot("texture_level_background_normal", *_render_targets.normal_tmp.get());

   // draw the atmospheric parts into the level texture using the atmosphere shader
   sf::Sprite tmp_sprite;
   const sf::Texture* tmp_sprite_texture = &_render_targets.level_background->getTexture();
#ifndef __EMSCRIPTEN__
   _atmosphere_shader->update();
   _render_targets.level->draw(tmp_sprite, sf::RenderStates{.texture = tmp_sprite_texture, .shader = &_atmosphere_shader->getShader()});
   takeScreenshot("texture_level_level_dist", *_render_targets.level.get());

   // draw the normal parts into the final normal texture using the atmosphere shader
   tmp_sprite_texture = &_render_targets.normal_tmp->getTexture();
   _atmosphere_shader->update();
   _render_targets.normal->draw(tmp_sprite, sf::RenderStates{.texture = tmp_sprite_texture, .shader = &_atmosphere_shader->getShader()});
   takeScreenshot("texture_level_background_normal_dist", *_render_targets.normal.get());

   drawGlowSprite();

   // draw the level layers into the level texture
   drawLayers(
      *_render_targets.level.get(),
      *_render_targets.normal.get(),
      static_cast<int32_t>(ZDepth::ForegroundMin),
      static_cast<int32_t>(ZDepth::ForegroundMax)
   );

   _light_system->drawDebug(*_render_targets.level.get());

   Gun::drawProjectileHitAnimations(*_render_targets.level.get());
   AnimationPlayer::getInstance().draw(*_render_targets.level.get());
   _level_script.draw(*_render_targets.level.get());

   drawDebugInformation();

   displayFinalTextures();

   drawLightMap();

   _light_system->draw(
      *_render_targets.deferred.get(), _render_targets.level, _render_targets.lighting, _render_targets.lighting2, _render_targets.normal
   );

   drawPostLightingLayers(*_render_targets.deferred.get());

   drawOverlayLayers(*_render_targets.deferred.get());

   _render_targets.deferred->display();

   takeScreenshot("texture_map_color", *_render_targets.level.get());
   takeScreenshot("texture_map_light", *_render_targets.lighting.get());
   takeScreenshot("texture_map_light2", *_render_targets.lighting2.get());
   takeScreenshot("texture_map_normal", *_render_targets.normal.get());
   takeScreenshot("texture_map_deferred", *_render_targets.deferred.get());

   const sf::Texture& level_deferred_texture = _render_targets.deferred->getTexture();
   sf::Sprite level_texture_sprite;

   level_texture_sprite.position = {_boom_effect._boom_offset_x, _boom_effect._boom_offset_y};
   level_texture_sprite.scale = {_render_targets.view_to_texture_scale, _render_targets.view_to_texture_scale};

   _gamma_shader->setTexture(level_deferred_texture);
   _gamma_shader->update();
   window->draw(level_texture_sprite, sf::RenderStates{.texture = &level_deferred_texture, .shader = &_gamma_shader->getGammaShader()});
#endif
}

void Level::updatePlayerLight()
{
   if (!_player_light->_enabled)
   {
      return;
   }

   _player_light->_pos_m = PlayerRegistry::getFirst()->getBody()->GetPosition();
   _player_light->updateSpritePosition();

   // zero alpha on death to avoid glitches as the player sinks down
   auto color = _player_light->_color;
   if (PlayerRegistry::getFirst()->isDead())
   {
      color.a = 0;
   }
   _player_light->_sprite->color = color;
}

const std::shared_ptr<LightSystem>& Level::getLightSystem() const
{
   return _light_system;
}

const std::shared_ptr<LightSystem::LightInstance>& Level::getPlayerLight() const
{
   return _player_light;
}

void Level::update(const sf::Time& dt)
{
   Projectile::update(dt);

   updateMechanismVolumes();
   updateCameraSystem(dt);
   updateViews();

   // clear conveyor belt state BEFORE updating the world
   // i.e. all objects on the belt are cleared here, then in Step() they are re-collected
   ConveyorBelt::resetBeltState();

   _world->Step(PhysicsConfiguration::getInstance()._time_step, 8, 3);
   GameContactListener::getInstance().processEvents();

   CameraPanorama::getInstance().update();
   _boom_effect.update(dt);

   AnimationPlayer::getInstance().update(dt);

   for (auto& tile_map : _tile_maps)
   {
      tile_map->update(dt);
   }

   const auto& player_chunk = PlayerRegistry::getFirst()->getChunk();

#ifdef DEVELOPMENT_MODE
   if (_mechanism_profiling_enabled)
   {
      timing_data.clear();
   }
#endif

   for (auto* mechanism_vector : _mechanism_registry.getList())
   {
      for (const auto& mechanism : *mechanism_vector)
      {
         if (checkUpdateMechanism(player_chunk, mechanism))
         {
#ifdef DEVELOPMENT_MODE
            if (_mechanism_profiling_enabled)
            {
               const auto mechanism_name = std::string{mechanism->objectName()};
               const auto time_start = std::chrono::high_resolution_clock::now();
               mechanism->update(dt);
               timing_data[mechanism_name].addUpdateTime(std::chrono::high_resolution_clock::now() - time_start);
            }
            else
            {
               mechanism->update(dt);
            }
#else
               mechanism->update(dt);
#endif
         }
      }
   }

   for (auto& layer : _mechanism_registry.getImageLayers())
   {
      layer->update(dt);
   }

   _level_script.update(dt);

   LuaInterface::instance().update(
      dt, [&player_chunk](const std::shared_ptr<GameMechanism>& mechanism) { return checkUpdateMechanism(player_chunk, mechanism); }
   );

   updatePlayerLight();

   _volume_updater->setRoomId(RoomUpdater::getCurrentId());
   _volume_updater->update();
   _volume_updater->updateProjectiles(Projectile::getProjectiles());
}

const std::shared_ptr<b2World>& Level::getWorld() const
{
   return _world;
}

void Level::addChainToWorld(const std::vector<b2Vec2>& chain, ObjectType object_type)
{
   if (fabs(chain[0].x - chain[chain.size() - 1].x) < 0.001f && fabs(chain[0].y - chain[chain.size() - 1].y) < 0.001f)
   {
      Log::Error() << "chain has equal start and end position" << std::endl;
      exit(0);
   }

   // it's easier to store all the physics chains in a separate data structure
   // than to parse the box2d world every time we want those loops.
   _world_chains.push_back(chain);

   b2ChainShape chain_shape;
   chain_shape.CreateLoop(&chain.at(0), static_cast<int32_t>(chain.size()));

   b2FixtureDef fixture_def;
   fixture_def.density = 0.0f;
   fixture_def.friction = 0.2f;
   fixture_def.shape = &chain_shape;

   b2BodyDef body_def;
   body_def.position.Set(0, 0);
   body_def.type = b2_staticBody;

   auto body = _world->CreateBody(&body_def);
   auto fixture = body->CreateFixture(&fixture_def);
   auto object_data = new FixtureNode(this);
   object_data->setObjectId(std::format("world_chain_{}", _world_chains.size() - 1));
   object_data->setType(object_type);
   fixture->SetUserData(static_cast<void*>(object_data));
}

void Level::addPathsToWorld(int32_t offset_x, int32_t offset_y, const std::vector<SquareMarcher::Path>& paths, ObjectType behavior)
{
   // create the physical chain with 1 body per chain
   for (const auto& path : paths)
   {
      std::vector<b2Vec2> chain(path._scaled.size());

      std::transform(
         path._scaled.begin(),
         path._scaled.end(),
         chain.begin(),
         [&](const auto& pos) { return b2Vec2((pos.x + offset_x) * PIXELS_PER_TILE / PPM, (pos.y + offset_y) * PIXELS_PER_TILE / PPM); }
      );

      addChainToWorld(chain, behavior);
   }
}

void Level::parseObj(const std::shared_ptr<TmxLayer>& layer, ObjectType behavior, const std::filesystem::path& path)
{
   std::vector<b2Vec2> points;
   std::vector<std::vector<uint32_t>> faces;
   Mesh::readObj(path.string(), points, faces);
   for (const auto& face : faces)
   {
      std::vector<b2Vec2> chain;
      std::vector<sf::Vector2f> debug_path;
      for (auto index : face)
      {
         const auto& p = points[index];
         const auto v = b2Vec2{(p.x + layer->_offset_x_px) / PPM, (p.y + layer->_offset_y_px) / PPM};

         if (_winding == Winding::Clockwise)
         {
            // box2D 2.4 changed the winding to clockwise, anything CCW will be fall-through
            chain.insert(chain.begin(), v);
         }
         else
         {
            chain.push_back(v);
         }

         debug_path.emplace_back(p.x / PIXELS_PER_TILE, p.y / PIXELS_PER_TILE);
      }

      // creating a box2d loop is automatically closing the path
      chain.pop_back();
      addChainToWorld(chain, behavior);

      // this should become a function callable from the console
      if (false)
      {
         Mesh::writeVerticesToImage(
            points,
            faces,
            {static_cast<int32_t>(layer->_width_tl * PIXELS_PER_TILE) / 4, static_cast<int32_t>(layer->_height_tl * PIXELS_PER_TILE) / 4},
            path.filename().string() + "_dump.png",
            0.25f
         );
      }
   }
}

void Level::regenerateLevelPaths(
   const std::shared_ptr<TmxLayer>& layer,
   const std::shared_ptr<TmxTileSet>& tileset,
   const std::filesystem::path& base_path,
   ParseData* parse_data,
   auto path_solid_optimized
)
{
   const auto path_solid_not_optimized = base_path / std::filesystem::path(parse_data->filename_obj_not_optimized);

   // dump the tileset into an obj file, optimise that and load it
   if (_physics.dumpObj(layer, tileset, path_solid_not_optimized))
   {
      Log::Info() << "optimising: " << path_solid_not_optimized;

      PathMerge::PathMerger merger;
      merger.loadObj(path_solid_not_optimized.string());
      const auto stats = merger.saveObj(path_solid_optimized.string());

      Log::Info() << "optimised: points " << stats.points_in << " -> " << stats.points_out << ", faces " << stats.faces_in << " -> "
                  << stats.faces_out;
   }
   else
   {
      Log::Error() << "dumping unoptimized obj (" << path_solid_not_optimized << ") failed";
   }

   parseObj(layer, parse_data->object_type, path_solid_optimized);
}

void Level::parsePhysicsTiles(
   const std::shared_ptr<TmxLayer>& layer,
   const std::shared_ptr<TmxTileSet>& tileset,
   const std::filesystem::path& base_path
)
{
   ParseData level_pd;
   level_pd.filename_obj_optimized = "layer_" + layer->_name + "_solid.obj";
   level_pd.filename_obj_not_optimized = "layer_" + layer->_name + "_solid_not_optimised.obj";
   level_pd.filename_physics_path_csv = "physics_path_solid.csv";
   level_pd.filename_grid_image = "physics_grid_solid.png";
   level_pd.filename_path_image = "physics_path_solid.png";
   level_pd.object_type = ObjectTypeSolid;
   level_pd.colliding_tiles = {1};

   ParseData solid_onesided_pd;
   solid_onesided_pd.filename_obj_optimized = "layer_" + layer->_name + "_solid_onesided.obj";
   solid_onesided_pd.filename_obj_not_optimized = "layer_" + layer->_name + "_solid_onesided_not_optimised.obj";
   solid_onesided_pd.filename_physics_path_csv = "physics_path_solid_onesided.csv";
   solid_onesided_pd.filename_grid_image = "physics_grid_solid_onesided.png";
   solid_onesided_pd.filename_path_image = "physics_path_solid_onesided.png";
   solid_onesided_pd.object_type = ObjectTypeSolidOneWay;
   solid_onesided_pd.colliding_tiles = {1};

   ParseData* parse_data = nullptr;

   if (layer->_name == "level")
   {
      parse_data = &level_pd;
   }
   else if (layer->_name == "level_solid_onesided")
   {
      parse_data = &solid_onesided_pd;
   }

   if (!parse_data)
   {
      return;
   }

   auto path_solid_optimized = base_path / std::filesystem::path(parse_data->filename_obj_optimized);

   Log::Info() << "loading: " << path_solid_optimized.make_preferred().generic_string();

   if (std::filesystem::exists(path_solid_optimized))
   {
      parseObj(layer, parse_data->object_type, path_solid_optimized);
   }
   else
   {
      regenerateLevelPaths(layer, tileset, base_path, parse_data, path_solid_optimized);
   }

   ChainShapeAnalyzer::analyze(_world);
}

const sf::Vector2f& Level::getStartPosition() const
{
   return _start_position;
}

#ifdef DEVELOPMENT_MODE
std::vector<MechanismSample> Level::getMechanismTimings(int32_t top_n) const
{
   std::vector<MechanismSample> samples;
   samples.reserve(timing_data.size());
   for (const auto& [name, timing] : timing_data)
   {
      samples.push_back({name, timing.getAverageUpdateMs(), timing.getAverageDrawMs()});
   }
   std::ranges::sort(
      samples,
      [](const auto& left_sample, const auto& right_sample)
      { return (left_sample.update_ms + left_sample.draw_ms) > (right_sample.update_ms + right_sample.draw_ms); }
   );
   if (static_cast<int32_t>(samples.size()) > top_n)
   {
      samples.resize(top_n);
   }
   return samples;
}

void Level::setMechanismProfilingEnabled(bool enabled)
{
   _mechanism_profiling_enabled = enabled;
}
#endif
