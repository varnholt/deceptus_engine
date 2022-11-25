#include "level.h"

// game
#include "animationplayer.h"
#include "camerapanorama.h"
#include "cameraroomlock.h"
#include "chainshapeanalyzer.h"
#include "constants.h"
#include "debugdraw.h"
#include "displaymode.h"
#include "effects/dust.h"
#include "extraitem.h"
#include "extramanager.h"
#include "fixturenode.h"
#include "framework/math/maptools.h"
#include "framework/math/sfmlmath.h"
#include "framework/tools/checksum.h"
#include "framework/tools/globalclock.h"
#include "framework/tools/log.h"
#include "framework/tools/timer.h"
#include "gameconfiguration.h"
#include "gamecontactlistener.h"
#include "gamedeserializedata.h"
#include "gamemechanismdeserializer.h"
#include "gamemechanismdeserializerconstants.h"
#include "gun.h"
#include "leveldescription.h"
#include "ingamemenumap.h"
#include "luainterface.h"
#include "mechanisms/bouncer.h"
#include "mechanisms/checkpoint.h"
#include "mechanisms/conveyorbelt.h"
#include "mechanisms/door.h"
#include "mechanisms/lever.h"
#include "meshtools.h"
#include "physics/physicsconfiguration.h"
#include "player/player.h"
#include "savestate.h"
#include "screentransition.h"
#include "squaremarcher.h"
#include "stenciltilemap.h"
#include "texturepool.h"
#include "tilemap.h"
#include "tilemapfactory.h"
#include "weather.h"

// sfml
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

#include "framework/tmxparser/tmxelement.h"
#include "framework/tmxparser/tmximage.h"
#include "framework/tmxparser/tmximagelayer.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxobjectgroup.h"
#include "framework/tmxparser/tmxparser.h"
#include "framework/tmxparser/tmxpolygon.h"
#include "framework/tmxparser/tmxpolyline.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxtileset.h"
#include "framework/tmxparser/tmxtools.h"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#ifdef __GNUC__
#define FMT_HEADER_ONLY
#include <fmt/core.h>
#else
#include <format>
namespace fmt = std;
#endif

// things that should be optimised
// - the tilemaps are unsorted, sort them by z once after deserializing a level

Level* Level::__current_level = nullptr;

//-----------------------------------------------------------------------------
std::string Level::getDescriptionFilename() const
{
   return _description_filename;
}

//-----------------------------------------------------------------------------
void Level::setDescriptionFilename(const std::string& description_filename)
{
   _description_filename = description_filename;
}

//-----------------------------------------------------------------------------
const Atmosphere& Level::getAtmosphere() const
{
   return _atmosphere;
}

//-----------------------------------------------------------------------------
void Level::initializeTextures()
{
   const auto& game_config = GameConfiguration::getInstance();

   // since stencil buffers are used, it is required to enable them explicitly
   sf::ContextSettings stencil_context_settings;
   stencil_context_settings.stencilBits = 8;
   // stencil_context_settings.antialiasingLevel = 8; // makes texture tearing much more reproducable

   _render_texture_level_background.reset();

   _atmosphere_shader.reset();
   _gamma_shader.reset();
   _blur_shader.reset();

   // this the render texture size derived from the window dimensions. as opposed to the window
   // dimensions this one takes the view dimensions into regard and preserves an integer multiplier
   const auto ratio_width = game_config._video_mode_width / game_config._view_width;
   const auto ratio_height = game_config._video_mode_height / game_config._view_height;
   const auto size_ratio = std::min(ratio_width, ratio_height);
   _view_to_texture_scale = 1.0f / size_ratio;

   const auto texture_width = static_cast<int32_t>(size_ratio * game_config._view_width);
   const auto texture_height = static_cast<int32_t>(size_ratio * game_config._view_height);

   _render_texture_level_background = std::make_shared<sf::RenderTexture>();
   _render_texture_level_background->create(static_cast<uint32_t>(texture_width), static_cast<uint32_t>(texture_height));

   _render_texture_level = std::make_shared<sf::RenderTexture>();
   _render_texture_level->create(
      static_cast<uint32_t>(texture_width),
      static_cast<uint32_t>(texture_height),
      stencil_context_settings  // the lights require stencils
   );

   _render_texture_lighting = std::make_shared<sf::RenderTexture>();
   _render_texture_lighting->create(
      static_cast<uint32_t>(texture_width),
      static_cast<uint32_t>(texture_height),
      stencil_context_settings  // the lights require stencils
   );

   _render_texture_normal = std::make_shared<sf::RenderTexture>();
   _render_texture_normal->create(static_cast<uint32_t>(texture_width), static_cast<uint32_t>(texture_height));

   _render_texture_deferred = std::make_shared<sf::RenderTexture>();
   _render_texture_deferred->create(static_cast<uint32_t>(texture_width), static_cast<uint32_t>(texture_height));

   _atmosphere_shader = std::make_unique<AtmosphereShader>(texture_width, texture_height);
   _gamma_shader = std::make_unique<GammaShader>();
   _blur_shader = std::make_unique<BlurShader>(texture_width, texture_height);

   // keep track of those textures
   _render_textures.clear();
   _render_textures.push_back(_render_texture_level);
   _render_textures.push_back(_render_texture_level_background);
   _render_textures.push_back(_render_texture_lighting);
   _render_textures.push_back(_render_texture_normal);
   _render_textures.push_back(_render_texture_deferred);

   for (const auto& fb : _render_textures)
   {
      Log::Info() << "created render texture: " << fb->getSize().x << " x " << fb->getSize().y;
   }

   _atmosphere_shader->initialize();
   _gamma_shader->initialize();
   _blur_shader->initialize();
}

//-----------------------------------------------------------------------------
Level::Level() : GameNode(nullptr)
{
   setClassName(typeid(Level).name());

   // init world for this level
   b2Vec2 gravity(0.f, PhysicsConfiguration::getInstance()._gravity);

   LuaInterface::instance().reset();

   // clear those here so the world destructor doesn't double-delete them
   Projectile::clear();

   _world = std::make_shared<b2World>(gravity);

   GameContactListener::getInstance().reset();
   _world->SetContactListener(&GameContactListener::getInstance());

   __current_level = this;

   _light_system = std::make_shared<LightSystem>();
   _static_light = std::make_shared<StaticLight>();

   // add raycast light for player
   _player_light = LightSystem::createLightInstance(Player::getCurrent(), {});
   _player_light->_color = sf::Color(255, 255, 255, 10);
   _light_system->_lights.push_back(_player_light);

   _mechanisms_list = {
      &_mechanism_bouncers,        &_mechanism_bubble_cubes,   &_mechanism_checkpoints,    &_mechanism_collapsing_platforms,
      &_mechanism_controller_help, &_mechanism_conveyor_belts, &_mechanism_crushers,       &_mechanism_death_blocks,
      &_mechanism_dialogues,       &_mechanism_doors,          &_mechanism_dust,           &_mechanism_fans,
      &_mechanism_lasers,          &_mechanism_levers,         &_mechanism_moveable_boxes, &_mechanism_on_off_blocks,
      &_mechanism_platforms,       &_mechanism_portals,        &_mechanism_ropes,          &_mechanism_rotating_blades,
      &_mechanism_sensor_rects,    &_mechanism_shader_layers,  &_mechanism_spike_balls,    &_mechanism_spike_blocks,
      &_mechanism_spikes,          &_mechanism_weather,
   };

   _mechanisms_map[std::string{layer_name_bouncers}] = &_mechanism_bouncers;
   _mechanisms_map[std::string{layer_name_bubble_cube}] = &_mechanism_bubble_cubes;
   _mechanisms_map[std::string{layer_name_checkpoints}] = &_mechanism_checkpoints;
   _mechanisms_map[std::string{layer_name_collapsing_platforms}] = &_mechanism_collapsing_platforms;
   _mechanisms_map[std::string{layer_name_controller_help}] = &_mechanism_controller_help;
   _mechanisms_map[std::string{layer_name_conveyorbelts}] = &_mechanism_conveyor_belts;
   _mechanisms_map[std::string{layer_name_crushers}] = &_mechanism_crushers;
   _mechanisms_map[std::string{layer_name_death_blocks}] = &_mechanism_death_blocks;
   _mechanisms_map[std::string{layer_name_dialogues}] = &_mechanism_dialogues;
   _mechanisms_map[std::string{layer_name_doors}] = &_mechanism_doors;
   _mechanisms_map[std::string{layer_name_dust}] = &_mechanism_dust;
   _mechanisms_map[std::string{layer_name_fans}] = &_mechanism_fans;
   _mechanisms_map[std::string{layer_name_lasers}] = &_mechanism_lasers;
   _mechanisms_map[std::string{layer_name_levers}] = &_mechanism_levers;
   _mechanisms_map[std::string{layer_name_moveable_objects}] = &_mechanism_moveable_boxes;
   _mechanisms_map[std::string{layer_name_on_off_blocks}] = &_mechanism_on_off_blocks;
   _mechanisms_map[std::string{layer_name_platforms}] = &_mechanism_platforms;
   _mechanisms_map[std::string{layer_name_portals}] = &_mechanism_portals;
   _mechanisms_map[std::string{layer_name_ropes}] = &_mechanism_ropes;
   _mechanisms_map[std::string{layer_name_rotating_blades}] = &_mechanism_rotating_blades;
   _mechanisms_map[std::string{layer_name_sensor_rects}] = &_mechanism_sensor_rects;
   _mechanisms_map[std::string{layer_name_shader_quads}] = &_mechanism_shader_layers;
   _mechanisms_map[std::string{layer_name_spike_balls}] = &_mechanism_spike_balls;
   _mechanisms_map[std::string{layer_name_spike_blocks}] = &_mechanism_spike_blocks;
   _mechanisms_map[std::string{layer_name_interval_spikes}] = &_mechanism_spikes;
   _mechanisms_map[std::string{layer_name_weather}] = &_mechanism_weather;

   // called whenever the player toggles a mechanism in the game
   Player::getCurrent()->setToggleCallback(
      [this]()
      {
         for (auto& door : _mechanism_doors)
         {
            std::dynamic_pointer_cast<Door>(door)->toggleWithPlayerChecks();
         }

         for (auto& lever : _mechanism_levers)
         {
            std::dynamic_pointer_cast<Lever>(lever)->toggle();
         }
      }
   );
}

//-----------------------------------------------------------------------------
Level::~Level()
{
   Log::Info() << "deleting current level";

   // stop active timers because their callbacks being called after destruction of the level/world can be nasty
   for (auto& enemy : _enemies)
   {
      Timer::removeByCaller(enemy);
   }

   // properly delete point map
   for (auto& kv : _point_map)
   {
      delete kv.second;
   }
}

//-----------------------------------------------------------------------------
void Level::deserializeParallaxMap(const std::shared_ptr<TmxLayer>& layer, const std::shared_ptr<TileMap>& tile_map)
{
   if (layer->_properties)
   {
      auto z_index = 0;
      auto parallax_factor_x = 1.0f;
      auto parallax_factor_y = 1.0f;
      auto parallax_offset_x = 0.0f;
      auto parallax_offset_y = 0.0f;
      auto& map = layer->_properties->_map;

      const auto& it_parallax_x_value = map.find("factor_x");
      if (it_parallax_x_value != map.end())
      {
         parallax_factor_x = it_parallax_x_value->second->_value_float.value();
      }

      const auto& it_parallax_y_value = map.find("factor_y");
      if (it_parallax_y_value != map.end())
      {
         parallax_factor_y = it_parallax_y_value->second->_value_float.value();
      }

      const auto& it_offset_x_value = map.find("offset_x_px");
      if (it_offset_x_value != map.end())
      {
         parallax_offset_x = static_cast<float>(it_offset_x_value->second->_value_int.value());
      }

      const auto& it_offset_y_value = map.find("offset_y_px");
      if (it_offset_y_value != map.end())
      {
         parallax_offset_y = static_cast<float>(it_offset_y_value->second->_value_int.value());
      }

      const auto& it_z_index_value = map.find("z");
      if (it_z_index_value != map.end())
      {
         z_index = it_z_index_value->second->_value_int.value();
      }

      // set up parallax layer with given properties
      const auto& it_parallax_view = map.find("slot");
      if (it_parallax_view != map.end())
      {
         const auto slot = it_parallax_view->second->_value_int.value();

         auto& parallax_layer = _parallax_layers[slot];

         parallax_layer._used = true;
         parallax_layer._z_index = z_index;
         parallax_layer._factor.x = parallax_factor_x;
         parallax_layer._factor.y = parallax_factor_y;
         parallax_layer._offset.x = parallax_offset_x;
         parallax_layer._offset.y = parallax_offset_y;
         parallax_layer._tile_map = tile_map;

         // determine placement error
         //
         //  +------------------------------------+-------+-------+
         //  |                                    |xxxxxxx|       |
         //  |                                    |xxxxxxx|       |
         //  |                                    |xxxxxxx|       |
         //  +------------------------------------+-------+-------+
         // 0px                                 800px   900px 1000px
         //
         //  800px   *   0.9     = 720px
         //  offset      factor  = actual
         //
         //  800px   -   720px   = 80px error
         //  offset      actual  = error

         const auto& parallax_factor = parallax_layer._factor;
         auto parallax_offset_with_error = parallax_layer._offset;
         parallax_offset_with_error.x *= parallax_factor.x;
         parallax_offset_with_error.y *= parallax_factor.y;
         parallax_layer._error = parallax_layer._offset - parallax_offset_with_error;
      }
   }
}

//-----------------------------------------------------------------------------
void Level::loadTmx()
{
   static const std::string parallax_identifier = "parallax_";

   const auto path = std::filesystem::path(_description->_filename).parent_path();

   const auto checksum_old = Checksum::readChecksum(_description->_filename + ".crc");
   const auto checksum_new = Checksum::calcChecksum(_description->_filename);
   if (checksum_old != checksum_new)
   {
      Log::Warning() << "checksum mismatch, deleting cached data";
      std::filesystem::remove(path / "physics_grid_solid.png");
      std::filesystem::remove(path / "physics_path_deadly.csv");
      std::filesystem::remove(path / "physics_path_solid.csv");
      std::filesystem::remove(path / "physics_path_solid.png");
      std::filesystem::remove(path / "layer_level_solid_not_optimised.obj");
      Checksum::writeChecksum(_description->_filename + ".crc", checksum_new);
   }

   sf::Clock elapsed;

   // parse tmx
   Log::Info() << "parsing tmx: " << _description->_filename;

   TmxParser tmx_parser;
   tmx_parser.parse(_description->_filename);

   Log::Info() << "parsing tmx, done within " << elapsed.getElapsedTime().asSeconds() << "s";
   elapsed.restart();

   Log::info("loading tmx... ");

   GameDeserializeData data;
   data._world = _world;
   data._base_path = path;

   GameMechanismDeserializer::deserialize(tmx_parser, this, data, _mechanisms_map);

   // process everything that's not considered a mechanism
   const auto& tmx_elements = tmx_parser.getElements();
   for (auto element : tmx_elements)
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

         const auto tile_map = TileMapFactory::makeTileMap(layer);
         tile_map->load(layer, tileset, path);
         auto push_tile_map = true;

         if (layer->_name == "atmosphere")
         {
            _atmosphere._tile_map = tile_map;
            _atmosphere.parse(layer, tileset);
         }
         else if (layer->_name == "extras")
         {
            const auto& extra_manager = Player::getCurrent()->getExtraManager();
            extra_manager->_tilemap = tile_map;
            extra_manager->load(layer, tileset);
         }
         else if (layer->_name.compare(0, parallax_identifier.length(), parallax_identifier) == 0)
         {
            deserializeParallaxMap(layer, tile_map);
            push_tile_map = false;
         }
         else if (layer->_name == "level" || layer->_name == "level_solid_onesided" || layer->_name == "level_deadly")
         {
            parsePhysicsTiles(layer, tileset, path);
         }

         if (push_tile_map)
         {
            _tile_maps.push_back(tile_map);
         }
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
            else if (object_group->_name == "extras")
            {
               const auto& extra_manager = Player::getCurrent()->getExtraManager();
               extra_manager->deserialize(this, data);
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
            else if (object_group->_name.compare(0, StaticLight::__layer_name.size(), StaticLight::__layer_name) == 0)
            {
               const auto light = StaticLight::deserialize(this, data);
               _static_light->_lights.push_back(light);
            }
         }
      }

      // parse images
      else if (element->_type == TmxElement::Type::TypeImageLayer)
      {
         const auto image = ImageLayer::deserialize(element, path);
         _image_layers.push_back(image);
      }
   }

   TileMapFactory::merge(_tile_maps);

   if (!_atmosphere._tile_map)
   {
      Log::Error() << "fatal: no physics layer (called 'physics') found!";
   }

   Log::Info() << "loading tmx, done within " << elapsed.getElapsedTime().asSeconds() << "s";
}

//-----------------------------------------------------------------------------
BoomEffect& Level::getBoomEffect()
{
   return _boom_effect;
}

//-----------------------------------------------------------------------------
bool Level::load()
{
   const auto level_json_path = std::filesystem::path(_description->_filename);
   setObjectId(_description->_filename);

   if (!std::filesystem::exists(level_json_path))
   {
      return false;
   }

   // load tmx
   loadTmx();

   // loading ao
   Log::Info() << "loading ao... ";
   _ambient_occlusion.load(level_json_path.parent_path(), std::filesystem::path(_description->_filename).stem().string());

   Log::Info() << "level loading complete";

   return true;
}

//-----------------------------------------------------------------------------
void Level::initialize()
{
   createViews();

   _description = LevelDescription::load(_description_filename);

   if (!_description)
   {
      Log::Error() << "level configuration is bad";
      return;
   }

   if (!load())
   {
      Log::Error() << "level loading failed";
      return;
   }

   _start_position.x = static_cast<float_t>(_description->_start_position.at(0) * PIXELS_PER_TILE + PLAYER_ACTUAL_WIDTH / 2);
   _start_position.y = static_cast<float_t>(_description->_start_position.at(1) * PIXELS_PER_TILE + DIFF_PLAYER_TILE_TO_PHYSICS);

   loadState();
   spawnEnemies();

   // dump();
}

//-----------------------------------------------------------------------------
void Level::loadState()
{
   const auto& save_state = SaveState::getCurrent();
   auto checkpoint_index = save_state._checkpoint;
   auto checkpoint = Checkpoint::getCheckpoint(checkpoint_index, _mechanism_checkpoints);

   if (checkpoint)
   {
      auto pos = checkpoint->calcCenter();
      _start_position.x = static_cast<float>(pos.x);
      _start_position.y = static_cast<float>(pos.y);
      Log::Info() << "move to checkpoint: " << checkpoint_index;
   }
   else
   {
      Log::Error() << "level doesn't have a start check point set up";
   }

   // deserialize mechanisms
   //
   //      "levelstate": {
   //          "data/catacombs/catacombs.tmx": {
   //              "lever_lever_spike_01": {
   //                  "state": -1
   //              }
   //          }
   //      },

   if (!save_state._level_state.is_null())
   {
      const auto& level_json = save_state._level_state[_description->_filename];

      if (!level_json.is_null())
      {
         for (auto& [mechanism_key, mechanism_values] : level_json.items())
         {
            const auto& mechanism_it = _mechanisms_map.find(mechanism_key);
            if (mechanism_it == _mechanisms_map.end())
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
                     auto game_node = dynamic_cast<GameNode*>(object.get());
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
   }
}

//-----------------------------------------------------------------------------
void Level::saveState()
{
   auto& j = SaveState::getCurrent()._level_state;

   nlohmann::json mechanisms_json;

   for (auto& [key, mechanisms] : _mechanisms_map)
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

//-----------------------------------------------------------------------------
void Level::reset()
{
   for (auto& door : _mechanism_doors)
   {
      door.reset();
   }
}

//-----------------------------------------------------------------------------
void Level::spawnEnemies()
{
   // deprecated approach:
   // merge enemy layer from tmx with enemy info that's stored inside json
   // iterate through all enemies in the json
   for (auto& json_description : _description->_enemies)
   {
      Log::Warning() << "deprecated: define enemies inside your TMX instead of JSON";
      auto lua_node = LuaInterface::instance().addObject(this, std::string("data/scripts/enemies/") + json_description._script);

      // find matching enemy data from the tmx layer and retrieve the patrol path from there
      const auto& it = _enemy_data_from_tmx_layer.find(json_description._id);
      if (it != _enemy_data_from_tmx_layer.end())
      {
         // positions from the tmx are given in pixels, not tiles
         json_description._position_in_tiles = false;

         json_description._start_position.push_back(it->second._pixel_position.x);
         json_description._start_position.push_back(it->second._pixel_position.y);

         if (json_description._generate_path)
         {
            it->second.addPaths(_world_chains);
         }

         if (!it->second._pixel_path.empty())
         {
            json_description._path = it->second._pixel_path;
         }

         // merge properties from tmx with those loaded from json
         for (auto& property : it->second._properties)
         {
            json_description._properties.push_back(property);
         }
      }

      // initialize lua node and store enemy
      lua_node->_enemy_description = json_description;
      lua_node->initialize();
      _enemies.push_back(lua_node);
   }

   // those enemies that have a lua script associated inside the tmx layer don't need
   // additional information from json, those can just be spawned.
   // this should probably be the future and only approach how to handle enemy spawning.
   for (auto& it : _enemy_data_from_tmx_layer)
   {
      const auto script = it.second.findProperty("script");

      if (!script.has_value())
      {
         Log::Error() << "missing script definition";
         continue;
      }

      auto lua_node = LuaInterface::instance().addObject(this, std::string("data/scripts/enemies/") + script.value()._value);

      EnemyDescription json_description;
      json_description._position_in_tiles = false;
      json_description._start_position.push_back(it.second._pixel_position.x);
      json_description._start_position.push_back(it.second._pixel_position.y);

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
         lua_node->_z_index = std::stoi(z_index_property.value()._value);
      }

      // merge properties from tmx with those loaded from json
      for (auto& property : it.second._properties)
      {
         json_description._properties.push_back(property);
      }

      // initialize lua node and store enemy
      lua_node->_enemy_description = json_description;
      lua_node->initialize();
      _enemies.push_back(lua_node);
   }
}

//-----------------------------------------------------------------------------
void Level::drawStaticChains(sf::RenderTarget& target)
{
   for (auto& path : _atmosphere._outlines)
   {
      target.draw(&path.at(0), path.size(), sf::LineStrip);
   }
}

//-----------------------------------------------------------------------------
const std::shared_ptr<sf::View>& Level::getLevelView() const
{
   return _level_view;
}

//-----------------------------------------------------------------------------
void Level::createViews()
{
   auto& game_config = GameConfiguration::getInstance();

   // the view dimensions never change
   _view_width = static_cast<float>(game_config._view_width);
   _view_height = static_cast<float>(game_config._view_height);

   _level_view.reset();
   _level_view = std::make_shared<sf::View>();
   _level_view->reset(sf::FloatRect(0.0f, 0.0f, _view_width, _view_height));
   _level_view->setViewport(sf::FloatRect(0.0f, 0.0f, 1.0f, 1.0f));

   for (auto& parallax_layer : _parallax_layers)
   {
      parallax_layer._view.reset();
      parallax_layer._view = std::make_shared<sf::View>();
      parallax_layer._view->reset(sf::FloatRect(0.0f, 0.0f, _view_width, _view_height));
      parallax_layer._view->setViewport(sf::FloatRect(0.0f, 0.0f, 1.0f, 1.0f));
   }
}

//-----------------------------------------------------------------------------
void Level::updateViews()
{
   const auto& look_vector = CameraPanorama::getInstance().getLookVector();
   const auto& camera_system = CameraSystem::getInstance();

   const auto level_view_x = camera_system.getX() + look_vector.x;
   const auto level_view_y = camera_system.getY() + look_vector.y;
   const auto view_rect = sf::FloatRect{level_view_x, level_view_y, _view_width, _view_height};

   CameraRoomLock::setViewRect(view_rect);

   _level_view->reset(view_rect);

   for (const auto& parallax : _parallax_layers)
   {
      if (parallax._used)
      {
         parallax._view->reset(sf::FloatRect(
            level_view_x * parallax._factor.x + parallax._error.x,
            level_view_y * parallax._factor.y + parallax._error.y,
            _view_width,
            _view_height
         ));
      }
   }
}

//-----------------------------------------------------------------------------
void Level::updateRoom()
{
   _room_current = Room::find(Player::getCurrent()->getPixelPositionFloat(), _rooms);
}

//-----------------------------------------------------------------------------
void Level::syncRoom()
{
   _room_current = Room::find(Player::getCurrent()->getPixelPositionFloat(), _rooms);
   CameraRoomLock::setRoom(_room_current);
}

//-----------------------------------------------------------------------------
void Level::updateCameraSystem(const sf::Time& dt)
{
   auto& camera_system = CameraSystem::getInstance();

   // update room
   const auto prev_room = _room_current;
   updateRoom();

   // room changed
   if (prev_room != _room_current)
   {
      Log::Info() << "player moved to room: " << (_room_current ? _room_current->getObjectId() : "undefined") << " on side '"
                  << (_room_current ? static_cast<char>(_room_current->enteredDirection(Player::getCurrent()->getPixelPositionFloat()))
                                    : '?')
                  << "'";

      // will update the current room in both cases, either after the camera lock delay or instantly
      if (_room_current && _room_current->_camera_lock_delay.has_value())
      {
         _room_current->lockCamera();
      }
      else
      {
         CameraRoomLock::setRoom(_room_current);
      }

      // trigger transition effect
      // when level has been loaded, room changes certainly do not require a transition
      if (_room_synced && _room_current)
      {
         _room_current->startTransition();
      }
   }

   // update camera system
   if (!_room_current || !_room_current->_camera_locked)
   {
      camera_system.update(dt, _view_width, _view_height);
   }

   _room_synced = true;
}

//-----------------------------------------------------------------------------
void Level::drawNormalMap()
{
   auto tile_maps = _tile_maps;

   std::sort(tile_maps.begin(), tile_maps.end(), [](const auto& lhs, const auto& rhs) { return lhs->getZ() < rhs->getZ(); });

   //   static int32_t bump_map_save_counter = 0;
   //   bump_map_save_counter++;
   //   if (bump_map_save_counter % 60 == 0)
   //   {
   //      mNormalTexture->getTexture().copyToImage().saveToFile("normal_map.png");
   //   }
}

//-----------------------------------------------------------------------------
void Level::drawLightMap()
{
   _render_texture_lighting->clear();
   _render_texture_lighting->setView(*_level_view);
   _light_system->draw(*_render_texture_lighting, {});
   _render_texture_lighting->display();

   //   static int32_t light_map_save_counter = 0;
   //   light_map_save_counter++;
   //   if (light_map_save_counter % 60 == 0)
   //   {
   //      _render_texture_lighting->getTexture().copyToImage().saveToFile("light_map.png");
   //   }
}

//-----------------------------------------------------------------------------
void Level::drawLightAndShadows(sf::RenderTarget& target)
{
   target.setView(*_level_view);
   _light_system->draw(target, {});
}

//-----------------------------------------------------------------------------
void Level::drawParallaxMaps(sf::RenderTarget& target, int32_t z_index)
{
   for (const auto& parallax : _parallax_layers)
   {
      if (parallax._used && parallax._z_index == z_index)
      {
         target.setView(*parallax._view);
         target.draw(*parallax._tile_map);
      }
   }

   // restore level view
   target.setView(*_level_view);
}

//-----------------------------------------------------------------------------
void Level::drawPlayer(sf::RenderTarget& color, sf::RenderTarget& normal)
{
   auto player = Player::getCurrent();
   player->draw(color, normal);
}

//-----------------------------------------------------------------------------
void Level::drawLayers(sf::RenderTarget& target, sf::RenderTarget& normal, int32_t from, int32_t to)
{
   target.setView(*_level_view);
   normal.setView(*_level_view);

   for (auto z_index = from; z_index <= to; z_index++)
   {
      _static_light->drawToZ(target, {}, z_index);

      for (const auto& smoke : _smoke_effect)
      {
         smoke->drawToZ(target, {}, z_index);
      }

      drawParallaxMaps(*_render_texture_level_background.get(), z_index);

      // draw all tile maps
      for (auto& tile_map : _tile_maps)
      {
         if (tile_map->getZ() == z_index)
         {
            tile_map->draw(target, normal, {});
         }
      }

      // draw mechanisms
      for (const auto& mechanism_vector : _mechanisms_list)
      {
         for (const auto& mechanism : *mechanism_vector)
         {
            if (mechanism->getZ() == z_index)
            {
               mechanism->draw(target, *_render_texture_normal.get());
            }
         }
      }

      // draw enemies
      for (auto& enemy : _enemies)
      {
         if (enemy->_z_index == z_index)
         {
            enemy->draw(target);
         }
      }

      if (z_index == static_cast<int32_t>(ZDepth::Player))
      {
         // ambient occlusion
         _ambient_occlusion.draw(target);

         // draw player
         drawPlayer(target, *_render_texture_normal.get());
      }

      // draw image layers
      for (auto& layer : _image_layers)
      {
         if (layer->_z_index == z_index)
         {
            target.draw(layer->_sprite, {layer->_blend_mode});
         }
      }
   }
}

//-----------------------------------------------------------------------------
void Level::drawAtmosphereLayer(sf::RenderTarget& target)
{
   if (!_atmosphere._tile_map)
   {
      return;
   }

   _atmosphere._tile_map->setVisible(true);

   target.setView(*_level_view);
   target.draw(*_atmosphere._tile_map);

   _atmosphere._tile_map->setVisible(false);
}

//-----------------------------------------------------------------------------
void Level::drawBlurLayer(sf::RenderTarget& target)
{
   target.setView(*_level_view);

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
bool Level::isPhysicsPathClear(const sf::Vector2i& a_tl, const sf::Vector2i& b_tl) const
{
   auto blocks = [this](uint32_t x, uint32_t y) -> bool { return _physics._physics_map[(_physics._grid_width * y) + x] == 1; };

   return MapTools::lineCollide(a_tl.x, a_tl.y, b_tl.x, b_tl.y, blocks);
}

//----------------------------------------------------------------------------------------------------------------------
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

//----------------------------------------------------------------------------------------------------------------------
void Level::drawDebugInformation()
{
   if (DisplayMode::getInstance().isSet(Display::Debug))
   {
      drawStaticChains(*_render_texture_level.get());
      DebugDraw::debugBodies(*_render_texture_level.get(), this);
      DebugDraw::drawRect(*_render_texture_level.get(), Player::getCurrent()->getPixelRectInt());
      DebugDraw::debugHitboxes(*_render_texture_level.get());

      for (const auto& room : _rooms)
      {
         for (const auto& sub_room : room->_sub_rooms)
         {
            DebugDraw::drawRect(*_render_texture_level.get(), sub_room._rect, sf::Color::Yellow);
         }
      }
   }
}

//-----------------------------------------------------------------------------
void Level::displayTextures()
{
   // display the whole texture
   sf::View view(sf::FloatRect(
      0.0f, 0.0f, static_cast<float>(_render_texture_level->getSize().x), static_cast<float>(_render_texture_level->getSize().y)
   ));

   view.setViewport(sf::FloatRect(0.0f, 0.0f, 1.0f, 1.0f));

   _render_texture_level->setView(view);
   _render_texture_level->display();

   _render_texture_normal->setView(*_level_view);
   _render_texture_normal->display();
}

void Level::drawGlowLayer()
{
#ifdef GLOW_ENABLED
   _blur_shader->clearTexture();
   drawBlurLayer(*mBlurShader->getRenderTexture().get());
   _blur_shader->getRenderTexture()->display();
   takeScreenshot("screenshot_blur", *_blur_shader->getRenderTexture().get());
#endif
}

void Level::drawGlowSprite()
{
#ifdef GLOW_ENABLED
   sf::Sprite blur_sprite(_blur_shader->getRenderTexture()->getTexture());
   const auto down_scale_x =
      _blur_shader->getRenderTextureScaled()->getSize().x / static_cast<float>(mBlurShader->getRenderTexture()->getSize().x);
   const auto down_scale_y =
      _blur_shader->getRenderTextureScaled()->getSize().y / static_cast<float>(mBlurShader->getRenderTexture()->getSize().y);
   blur_sprite.scale({down_scale_x, down_scale_y});

   sf::RenderStates statesShader;
   _blur_shader->update();
   states_shader.shader = &mBlurShader->getShader();
   _blur_shader->getRenderTextureScaled()->draw(blur_sprite, statesShader);

   sf::Sprite blur_scale_sprite(mBlurShader->getRenderTextureScaled()->getTexture());
   blur_scale_sprite.scale(1.0f / down_scale_x, 1.0f / down_scale_y);
   blur_scale_sprite.setTextureRect(sf::IntRect(
      0,
      static_cast<int32_t>(blur_scale_sprite.getTexture()->getSize().y),
      static_cast<int32_t>(blur_scale_sprite.getTexture()->getSize().x),
      -static_cast<int32_t>(blur_scale_sprite.getTexture()->getSize().y)
   ));

   sf::RenderStates states_add;
   statesAdd.blendMode = sf::BlendAdd;
   _level_render_texture->draw(blur_scale_sprite, states_add);
#endif
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
void Level::draw(const std::shared_ptr<sf::RenderTexture>& window, bool screenshot)
{
   _screenshot = screenshot;

   // render atmosphere to atmosphere texture, that texture is used in the shader only
   _atmosphere_shader->getRenderTexture()->clear();
   drawAtmosphereLayer(*_atmosphere_shader->getRenderTexture().get());
   _atmosphere_shader->getRenderTexture()->display();
   takeScreenshot("texture_atmosphere", *_atmosphere_shader->getRenderTexture().get());

   // render glowing elements
   drawGlowLayer();

   // render layers affected by the atmosphere
   _render_texture_level_background->clear();
   _render_texture_normal->clear();

   drawLayers(
      *_render_texture_level_background.get(),
      *_render_texture_normal.get(),
      static_cast<int32_t>(ZDepth::BackgroundMin),
      static_cast<int32_t>(ZDepth::BackgroundMax)
   );
   _render_texture_level_background->display();
   takeScreenshot("texture_level_background", *_render_texture_level_background.get());

   // draw the atmospheric parts into the level texture using the atmosphere shader
   sf::Sprite background_sprite(_render_texture_level_background->getTexture());
   _atmosphere_shader->update();
   _render_texture_level->draw(background_sprite, &_atmosphere_shader->getShader());

   drawGlowSprite();

   // draw the level layers into the level texture
   drawLayers(
      *_render_texture_level.get(),
      *_render_texture_normal.get(),
      static_cast<int32_t>(ZDepth::ForegroundMin),
      static_cast<int32_t>(ZDepth::ForegroundMax)
   );

   Gun::drawProjectileHitAnimations(*_render_texture_level.get());
   AnimationPlayer::getInstance().draw(*_render_texture_level.get());

   drawDebugInformation();

   displayTextures();

   drawLightMap();

   _light_system->draw(*_render_texture_deferred.get(), _render_texture_level, _render_texture_lighting, _render_texture_normal);

   _render_texture_deferred->display();

   takeScreenshot("texture_map_color", *_render_texture_level.get());
   takeScreenshot("texture_map_light", *_render_texture_lighting.get());
   takeScreenshot("texture_map_normal", *_render_texture_normal.get());
   takeScreenshot("texture_map_deferred", *_render_texture_deferred.get());

   auto level_texture_sprite = sf::Sprite(_render_texture_deferred->getTexture());
   _gamma_shader->setTexture(_render_texture_deferred->getTexture());

   level_texture_sprite.setPosition(_boom_effect._boom_offset_x, _boom_effect._boom_offset_y);
   level_texture_sprite.scale(_view_to_texture_scale, _view_to_texture_scale);

   _gamma_shader->update();
   window->draw(level_texture_sprite, &_gamma_shader->getGammaShader());
}

//-----------------------------------------------------------------------------
void Level::updatePlayerLight()
{
   _player_light->_pos_m = Player::getCurrent()->getBody()->GetPosition();
   _player_light->updateSpritePosition();

   // the player, once he dies, becomes inactive and just sinks down
   // so the player light is disabled to avoid any glitches
   _player_light->_color = sf::Color(255, 255, 255, Player::getCurrent()->isDead() ? 0 : 10);
}

//-----------------------------------------------------------------------------
const std::shared_ptr<LightSystem>& Level::getLightSystem() const
{
   return _light_system;
}

//-----------------------------------------------------------------------------
void Level::update(const sf::Time& dt)
{
   Projectile::update(dt);

   updateCameraSystem(dt);
   updateViews();

   // clear conveyor belt state BEFORE updating the world
   // i.e. all objects on the belt are cleared here, then in Step() they are re-collected
   ConveyorBelt::resetBeltState();

   _world->Step(PhysicsConfiguration::getInstance()._time_step, 8, 3);

   CameraPanorama::getInstance().update();
   _boom_effect.update(dt);

   AnimationPlayer::getInstance().update(dt);

   for (auto& tile_map : _tile_maps)
   {
      tile_map->update(dt);
   }

   for (const auto& mechanism_vector : _mechanisms_list)
   {
      for (const auto& mechanism : *mechanism_vector)
      {
         mechanism->update(dt);
      }
   }

   LuaInterface::instance().update(dt);

   updatePlayerLight();

   _static_light->update(GlobalClock::getInstance().getElapsedTime());

   for (const auto& smoke : _smoke_effect)
   {
      smoke->update(GlobalClock::getInstance().getElapsedTime());
   }
}

//-----------------------------------------------------------------------------
const std::shared_ptr<b2World>& Level::getWorld() const
{
   return _world;
}

//-----------------------------------------------------------------------------
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
   object_data->setObjectId(fmt::format("world_chain_{}", _world_chains.size() - 1));
   object_data->setType(object_type);
   fixture->SetUserData(static_cast<void*>(object_data));
}

//-----------------------------------------------------------------------------
void Level::addPathsToWorld(int32_t offset_x, int32_t offset_y, const std::vector<SquareMarcher::Path>& paths, ObjectType behavior)
{
   // create the physical chain with 1 body per chain
   for (const auto& path : paths)
   {
      std::vector<b2Vec2> chain;
      for (const auto& pos : path._scaled)
      {
         chain.push_back({(pos.x + offset_x) * PIXELS_PER_TILE / PPM, (pos.y + offset_y) * PIXELS_PER_TILE / PPM});
      }

      addChainToWorld(chain, behavior);
   }
}

//-----------------------------------------------------------------------------
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
         chain.push_back({(p.x + layer->_offset_x_px) / PPM, (p.y + layer->_offset_y_px) / PPM});
         debug_path.push_back({p.x / PIXELS_PER_TILE, p.y / PIXELS_PER_TILE});
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

//-----------------------------------------------------------------------------
void Level::parsePhysicsTiles(
   const std::shared_ptr<TmxLayer>& layer,
   const std::shared_ptr<TmxTileSet>& tileset,
   const std::filesystem::path& base_path
)
{
   struct ParseData
   {
      std::string filename_obj_optimized;
      std::string filename_obj_not_optimized;
      std::string filename_physics_path_csv;
      std::string filename_grid_image;
      std::string filename_path_image;
      ObjectType object_type = ObjectTypeSolid;
      std::vector<int32_t> colliding_tiles;
   };

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

   ParseData deadly_pd;
   deadly_pd.filename_obj_optimized = "layer_" + layer->_name + "_deadly.obj";
   deadly_pd.filename_obj_not_optimized = "layer_" + layer->_name + "_deadly_not_optimised.obj";
   deadly_pd.filename_physics_path_csv = "physics_path_deadly.csv";
   deadly_pd.filename_grid_image = "physics_grid_deadly.png";
   deadly_pd.filename_path_image = "physics_path_deadly.png";
   deadly_pd.object_type = ObjectTypeDeadly;
   deadly_pd.colliding_tiles = {3};

   ParseData* pd = nullptr;

   if (layer->_name == "level")
   {
      pd = &level_pd;
   }
   else if (layer->_name == "level_solid_onesided")
   {
      pd = &solid_onesided_pd;
   }
   else if (layer->_name == "level_deadly")
   {
      pd = &deadly_pd;
   }

   if (!pd)
   {
      return;
   }

   static constexpr float scale = 1.0f / 3.0f;

   auto path_solid_optimized = base_path / std::filesystem::path(pd->filename_obj_optimized);

   Log::Info() << "loading: " << path_solid_optimized.make_preferred().generic_string();

   _physics.parse(layer, tileset, base_path);

   // this whole block should be generated by an external tool
   // right now the squaremarcher output is still used for the in-game map visualization
   SquareMarcher square_marcher(
      _physics._grid_width,
      _physics._grid_height,
      _physics._physics_map,
      pd->colliding_tiles,
      base_path / std::filesystem::path(pd->filename_physics_path_csv),
      scale
   );

   square_marcher.writeGridToImage(base_path / std::filesystem::path(pd->filename_grid_image));  // not needed
   square_marcher.writePathToImage(base_path / std::filesystem::path(pd->filename_path_image));  // needed from obj as well

   if (std::filesystem::exists(path_solid_optimized))
   {
      parseObj(layer, pd->object_type, path_solid_optimized);
   }
   else
   {
      const auto path_solid_not_optimized = base_path / std::filesystem::path(pd->filename_obj_not_optimized);

      // dump the tileset into an obj file, optimise that and load it
      if (_physics.dumpObj(layer, tileset, path_solid_not_optimized))
      {
#ifdef __linux__
         auto cmd =
            std::string("tools/path_merge/path_merge") + " " + path_solid_not_optimized.string() + " " + path_solid_optimized.string();
#else
         auto cmd = std::string("tools\\path_merge\\path_merge.exe") + " " + path_solid_not_optimized.string() + " " +
                    path_solid_optimized.string();
#endif

         Log::Info() << "running cmd: " << cmd;

         if (std::system(cmd.c_str()) != 0)
         {
            Log::Error() << "command failed";
         }
         else
         {
            Log::Info() << "command succeeded";
         }
      }
      else
      {
         Log::Error() << "dumping unoptimized obj (" << path_solid_not_optimized << ") failed";
      }

      // fallback to square marched level
      if (!std::filesystem::exists(path_solid_optimized))
      {
         Log::Warning() << "could not find " << path_solid_optimized.string() << ", obj generator failed";
         addPathsToWorld(layer->_offset_x_px, layer->_offset_y_px, square_marcher._paths, pd->object_type);
      }
      else
      {
         // parse the optimised obj
         parseObj(layer, pd->object_type, path_solid_optimized);
      }
   }

   //   // layer of deadly objects
   //   const auto pathDeadly = basePath / std::filesystem::path("layer_" + layer->mName + "_deadly.obj");
   //   if (std::filesystem::exists(pathDeadly))
   //   {
   //      parseObj(layer, ObjectType::ObjectTypeDeadly, pathDeadly);
   //   }
   //   else
   //   {
   //      SquareMarcher deadly(
   //         _physics._grid_width,
   //         _physics._grid_height,
   //         _physics._physics_map,
   //         std::vector<int32_t>{3},
   //         basePath / std::filesystem::path("physics_path_deadly.csv"),
   //         scale
   //      );
   //
   //      addPathsToWorld(layer->mOffsetX, layer->mOffsetY, deadly.mPaths, ObjectTypeDeadly);
   //   }

   ChainShapeAnalyzer::analyze(_world);
}

//-----------------------------------------------------------------------------
const sf::Vector2f& Level::getStartPosition() const
{
   return _start_position;
}

//-----------------------------------------------------------------------------
Level* Level::getCurrentLevel()
{
   return __current_level;
}

//-----------------------------------------------------------------------------
void Level::addDebugRect(void* body, float x, float y, float w, float h)
{
   auto points = new b2Vec2[4];

   points[0] = b2Vec2(x * MPP, y * MPP);
   points[1] = b2Vec2(x * MPP + w * MPP, y * MPP);
   points[2] = b2Vec2(x * MPP + w * MPP, y * MPP + h * MPP);
   points[3] = b2Vec2(x * MPP, y * MPP + h * MPP);

   _point_map[body] = points;
   _point_count_map[body] = 4;
}

//-----------------------------------------------------------------------------
std::shared_ptr<Portal> Level::getNearbyPortal() const
{
   std::shared_ptr<Portal> nearby_portal;

   for (auto& p : _mechanism_portals)
   {
      auto portal = std::dynamic_pointer_cast<Portal>(p);
      if (portal->isPlayerAtPortal())
      {
         nearby_portal = portal;
         break;
      }
   }

   return nearby_portal;
}

//-----------------------------------------------------------------------------
std::shared_ptr<Bouncer> Level::getNearbyBouncer() const
{
   std::shared_ptr<Bouncer> nearby_bouncer;

   for (auto& tmp : _mechanism_bouncers)
   {
      auto bouncer = std::dynamic_pointer_cast<Bouncer>(tmp);
      if (bouncer->isPlayerAtBouncer())
      {
         nearby_bouncer = bouncer;
         break;
      }
   }

   return nearby_bouncer;
}

//-----------------------------------------------------------------------------
const std::vector<std::shared_ptr<GameMechanism>>& Level::getCheckpoints() const
{
   return _mechanism_checkpoints;
}

//-----------------------------------------------------------------------------
const std::unordered_map<void*, size_t>& Level::getPointSizeMap() const
{
   return _point_count_map;
}

//-----------------------------------------------------------------------------
const std::unordered_map<void*, b2Vec2*>& Level::getPointMap() const
{
   return _point_map;
}
