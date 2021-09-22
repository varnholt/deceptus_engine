#include "level.h"

// game
#include "animationplayer.h"
#include "camerapane.h"
#include "checkpoint.h"
#include "constants.h"
#include "debugdraw.h"
#include "dialogue.h"
#include "displaymode.h"
#include "extraitem.h"
#include "extramanager.h"
#include "fixturenode.h"
#include "framework/math/maptools.h"
#include "framework/math/sfmlmath.h"
#include "framework/tools/checksum.h"
#include "framework/tools/globalclock.h"
#include "framework/tools/timer.h"
#include "gameconfiguration.h"
#include "gamecontactlistener.h"
#include "leveldescription.h"
#include "levelmap.h"
#include "luainterface.h"
#include "mechanisms/bouncer.h"
#include "mechanisms/conveyorbelt.h"
#include "mechanisms/crusher.h"
#include "mechanisms/deathblock.h"
#include "mechanisms/door.h"
#include "mechanisms/fan.h"
#include "mechanisms/laser.h"
#include "mechanisms/lever.h"
#include "mechanisms/moveablebox.h"
#include "mechanisms/movingplatform.h"
#include "mechanisms/rope.h"
#include "mechanisms/ropewithlight.h"
#include "mechanisms/spikeball.h"
#include "mechanisms/spikes.h"
#include "meshtools.h"
#include "physics/physicsconfiguration.h"
#include "player/player.h"
#include "savestate.h"
#include "squaremarcher.h"
#include "texturepool.h"
#include "tilemap.h"
#include "weather.h"

// sfml
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

#include "framework/tmxparser/tmxelement.h"
#include "framework/tmxparser/tmximage.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmximagelayer.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxobjectgroup.h"
#include "framework/tmxparser/tmxpolygon.h"
#include "framework/tmxparser/tmxpolyline.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxparser.h"
#include "framework/tmxparser/tmxtileset.h"
#include "framework/tmxparser/tmxtools.h"

// poly2tri
#include "poly2tri/poly2tri.h"
#include "poly2tri/common/shapes.h"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

// things that should be optimised
// - the tilemaps are unsorted, sort them by z once after deserializing a level


Level* Level::__current_level = nullptr;


//-----------------------------------------------------------------------------
std::string Level::getDescriptionFilename() const
{
   return _description_filename;
}


//-----------------------------------------------------------------------------
void Level::setDescriptionFilename(const std::string &descriptionFilename)
{
   _description_filename = descriptionFilename;
}


//-----------------------------------------------------------------------------
const Atmosphere& Level::getPhysics() const
{
   return _atmosphere;
}


//-----------------------------------------------------------------------------
void Level::initializeTextures()
{
   GameConfiguration& gameConfig = GameConfiguration::getInstance();

   // since stencil buffers are used, it is required to enable them explicitly
   sf::ContextSettings stencil_context_settings;
   stencil_context_settings.stencilBits = 8;

   _render_texture_level_background.reset();

   _atmosphere_shader.reset();
   _gamme_shader.reset();
   _blur_shader.reset();
   _death_shader.reset();

   // this the render texture size derived from the window dimensions. as opposed to the window
   // dimensions this one takes the view dimensions into regard and preserves an integer multiplier
   const auto ratio_width = gameConfig._video_mode_width / gameConfig._view_width;
   const auto ratio_height = gameConfig._video_mode_height / gameConfig._view_height;
   const auto size_ratio = std::min(ratio_width, ratio_height);
   _view_to_texture_scale = 1.0f / size_ratio;

   const auto texture_width = static_cast<int32_t>(size_ratio * gameConfig._view_width);
   const auto texture_height = static_cast<int32_t>(size_ratio * gameConfig._view_height);

   _render_texture_level_background = std::make_shared<sf::RenderTexture>();
   _render_texture_level_background->create(
      static_cast<uint32_t>(texture_width),
      static_cast<uint32_t>(texture_height)
   );

   _render_texture_level = std::make_shared<sf::RenderTexture>();
   _render_texture_level->create(
      static_cast<uint32_t>(texture_width),
      static_cast<uint32_t>(texture_height),
      stencil_context_settings // the lights require stencils
   );

   _render_texture_lighting = std::make_shared<sf::RenderTexture>();
   _render_texture_lighting->create(
      static_cast<uint32_t>(texture_width),
      static_cast<uint32_t>(texture_height),
      stencil_context_settings // the lights require stencils
   );

   _render_texture_normal = std::make_shared<sf::RenderTexture>();
   _render_texture_normal->create(
      static_cast<uint32_t>(texture_width),
      static_cast<uint32_t>(texture_height)
   );

   _render_texture_deferred = std::make_shared<sf::RenderTexture>();
   _render_texture_deferred->create(
      static_cast<uint32_t>(texture_width),
      static_cast<uint32_t>(texture_height)
   );

   _atmosphere_shader = std::make_unique<AtmosphereShader>(texture_width, texture_height);
   _gamme_shader = std::make_unique<GammaShader>();
   _blur_shader = std::make_unique<BlurShader>(texture_width, texture_height);
   _death_shader = std::make_unique<DeathShader>(texture_width, texture_height);

   // keep track of those textures
   _render_textures.clear();
   _render_textures.push_back(_render_texture_level);
   _render_textures.push_back(_render_texture_level_background);
   _render_textures.push_back(_render_texture_lighting);
   _render_textures.push_back(_render_texture_normal);
   _render_textures.push_back(_render_texture_deferred);

   for (const auto& fb : _render_textures)
   {
      std::cout << "[x] created render texture: " << fb->getSize().x << " x " << fb->getSize().y << std::endl;
   }

   _atmosphere_shader->initialize();
   _gamme_shader->initialize();
   _blur_shader->initialize();
   _death_shader->initialize();
}


//-----------------------------------------------------------------------------
Level::Level()
  : GameNode(nullptr)
{
   setName(typeid(Level).name());

   // init world for this level
   b2Vec2 gravity(0.f, PhysicsConfiguration::getInstance().mGravity);

   LuaInterface::instance()->reset();

   // clear those here so the world destructor doesn't double-delete them
   Projectile::clear();

   _world = std::make_shared<b2World>(gravity);

   GameContactListener::getInstance()->reset();
   _world->SetContactListener(GameContactListener::getInstance());

   __current_level = this;

   _light_system = std::make_shared<LightSystem>();
   _static_light = std::make_shared<StaticLight>();

   // add raycast light for player
   _player_light = LightSystem::createLightInstance();
   _player_light->_color = sf::Color(255, 255, 255, 10);
   _light_system->_lights.push_back(_player_light);

   _map = std::make_unique<LevelMap>();

   _mechanisms = {
      &_mechanism_bouncers,
      &_mechanism_conveyor_belts,
      &_mechanism_crushers,
      &_mechanism_death_blocks,
      &_mechanism_doors,
      &_mechanism_fans,
      &_mechanism_lasers,
      &_mechanism_levers,
      &_mechanism_moveable_boxes,
      &_mechanism_platforms,
      &_mechanism_portals,
      &_mechanism_ropes,
      &_mechanism_spike_balls,
      &_mechanism_spikes,
   };
}


//-----------------------------------------------------------------------------
Level::~Level()
{
   std::cout << "[x] deleting current level" << std::endl;

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

   // clear tmx elements
   for (auto tmx_element : _tmx_elements)
   {
      delete tmx_element;
   }

   _tmx_elements.clear();
}


//-----------------------------------------------------------------------------
void Level::deserializeParallaxMap(TmxLayer* layer)
{
   if (layer->_properties)
   {
      auto parallax = 1.0f;
      auto& map = layer->_properties->_map;
      auto itParallaxValue = map.find("parallax");
      if (itParallaxValue != map.end())
      {
         parallax = itParallaxValue->second->_value_float.value();
      }

      auto itParallaxView = map.find("parallax_view");
      if (itParallaxView != map.end())
      {
         const auto view = itParallaxView->second->_value_int.value();
         _parallax_factors[view] = parallax;
      }
   }
}


//-----------------------------------------------------------------------------
void Level::loadTmx()
{
   static const std::string parallaxIdentifier = "parallax_";

   auto path = std::filesystem::path(_description->_filename).parent_path();

   const auto checksum_old = Checksum::readChecksum(_description->_filename + ".crc");
   const auto checksum_new = Checksum::calcChecksum(_description->_filename);
   if (checksum_old != checksum_new)
   {
      std::cout << "[x] checksum mismatch, deleting cached data" << std::endl;
      std::filesystem::remove(path / "physics_grid_solid.png");
      std::filesystem::remove(path / "physics_path_deadly.csv");
      std::filesystem::remove(path / "physics_path_solid.csv");
      std::filesystem::remove(path / "physics_path_solid.png");
      std::filesystem::remove(path / "layer_level_solid_not_optimised.obj");
      Checksum::writeChecksum(_description->_filename + ".crc", checksum_new);
   }

   sf::Clock elapsed;

   // parse tmx
   std::cout << "[x] parsing tmx: " << _description->_filename << std::endl;

   _tmx_parser = std::make_unique<TmxParser>();
   _tmx_parser->parse(_description->_filename);

   std::cout << "[x] parsing tmx, done within " << elapsed.getElapsedTime().asSeconds() << "s" << std::endl;
   elapsed.restart();

   std::cout << "[x] loading tmx... " << std::endl;

   _tmx_elements = _tmx_parser->getElements();

   for (auto element : _tmx_elements)
   {
      if (element->_type == TmxElement::TypeLayer)
      {
         auto layer = dynamic_cast<TmxLayer*>(element);
         auto tileset = _tmx_parser->getTileSet(layer);

         if (layer->_name.rfind("doors", 0) == 0)
         {
            _mechanism_doors = Door::load(layer, tileset, path, _world);
         }
         else if (layer->_name == "fans")
         {
            Fan::load(layer, tileset, _world);
         }
         else if (layer->_name == "lasers")
         {
            const auto lasers = Laser::load(layer, tileset, path, _world);
            _mechanism_lasers.insert(_mechanism_lasers.end(), lasers.begin(), lasers.end());
         }
         else if (layer->_name == "lasers_2") // support for dstar's new laser tileset
         {
            const auto lasers = Laser::load(layer, tileset, path, _world);
            _mechanism_lasers.insert(_mechanism_lasers.end(), lasers.begin(), lasers.end());
         }
         else if (layer->_name == "levers")
         {
            _mechanism_levers = Lever::load(layer, tileset, path, _world);
         }
         else if (layer->_name == "platforms")
         {
            _mechanism_platforms = MovingPlatform::load(layer, tileset, path, _world);
         }
         else if (layer->_name == "portals")
         {
            _mechanism_portals = Portal::load(layer, tileset, path, _world);
         }
         else if (layer->_name == "toggle_spikes")
         {
            auto spikes = Spikes::load(layer, tileset, path, Spikes::Mode::Toggled);
            for (const auto &s : spikes)
            {
               _mechanism_spikes.push_back(s);
            }
         }
         else if (layer->_name == "trap_spikes")
         {
            auto spikes = Spikes::load(layer, tileset, path, Spikes::Mode::Trap);
            for (const auto &s : spikes)
            {
               _mechanism_spikes.push_back(s);
            }
         }
         else if (layer->_name == "interval_spikes")
         {
            auto spikes = Spikes::load(layer, tileset, path, Spikes::Mode::Interval);
            for (const auto &s : spikes)
            {
               _mechanism_spikes.push_back(s);
            }
         }
         else // tile map
         {
            std::shared_ptr<TileMap> tileMap = std::make_shared<TileMap>();
            tileMap->load(layer, tileset, path);

            auto pushTileMap = true;

            if (layer->_name == "atmosphere")
            {
               _atmosphere.mTileMap = tileMap;
               _atmosphere.parse(layer, tileset);
            }
            else if (layer->_name == "extras")
            {
               Player::getCurrent()->getExtraManager()->mTilemap = tileMap;
               Player::getCurrent()->getExtraManager()->load(layer, tileset);
            }
            else if (layer->_name.compare(0, parallaxIdentifier.length(), parallaxIdentifier) == 0)
            {
               deserializeParallaxMap(layer);
               _parallax_maps.push_back(tileMap);
               pushTileMap = false;
            }
            else if (layer->_name == "level" || layer->_name == "level_solid_onesided" || layer->_name == "level_deadly")
            {
               parsePhysicsTiles(layer, tileset, path);
            }

            if (pushTileMap)
            {
               _tile_maps.push_back(tileMap);
            }
         }
      }

      else if (element->_type == TmxElement::TypeObjectGroup)
      {
         TmxObjectGroup* object_group = dynamic_cast<TmxObjectGroup*>(element);

         for (const auto& object : object_group->_objects)
         {
            TmxObject* tmx_object = object.second;

            if (object_group->_name == "lasers" || object_group->_name == "lasers_2")
            {
               Laser::addObject(tmx_object);
            }
            else if (object_group->_name == "enemies")
            {
               Enemy enemy;
               enemy.parse(tmx_object);
               _enemy_data_from_tmx_layer[enemy._id]=enemy;
            }
            else if (object_group->_name == "fans")
            {
               Fan::addObject(tmx_object, path);
            }
            else if (object_group->_name == "portals")
            {
               if (tmx_object->_polyline)
               {
                  Portal::link(_mechanism_portals, tmx_object);
               }
            }
            else if (object_group->_name == "ropes")
            {
               auto rope = std::make_shared<Rope>(dynamic_cast<GameNode*>(this));
               rope->setup(tmx_object, _world);
               _mechanism_ropes.push_back(rope);
            }
            else if (object_group->_name == "ropes_with_light")
            {
               auto rope = std::make_shared<RopeWithLight>(dynamic_cast<GameNode*>(this));
               rope->setup(tmx_object, _world);
               _mechanism_ropes.push_back(rope);
            }
            else if (object_group->_name == "smoke")
            {
               auto smoke = SmokeEffect::deserialize(tmx_object, object_group);
               _smoke_effect.push_back(smoke);
            }
            else if (object_group->_name == "spike_balls")
            {
               auto spikeBall = std::make_shared<SpikeBall>(dynamic_cast<GameNode*>(this));
               spikeBall->setup(tmx_object, _world);
               _mechanism_spike_balls.push_back(spikeBall);
            }
            else if (object_group->_name == "moveable_objects")
            {
               auto box = std::make_shared<MoveableBox>(dynamic_cast<GameNode*>(this));
               box->setup(tmx_object, _world);
               _mechanism_moveable_boxes.push_back(box);
            }
            else if (object_group->_name == "death_blocks")
            {
               auto deathBlock = std::make_shared<DeathBlock>(dynamic_cast<GameNode*>(this));
               deathBlock->setup(tmx_object, _world);
               _mechanism_death_blocks.push_back(deathBlock);
            }
            else if (object_group->_name == "checkpoints")
            {
               const auto cpi = Checkpoint::add(tmx_object);
               auto cp = Checkpoint::getCheckpoint(cpi);

               // whenever we reach a checkpoint, update the checkpoint index in the save state
               cp->addCallback([cpi](){SaveState::getCurrent().mCheckpoint = cpi;});

               // whenever we reach a checkpoint, serialize the save state
               cp->addCallback([](){SaveState::serializeToFile();});
            }
            else if (object_group->_name == "dialogues")
            {
               Dialogue::add(tmx_object);
            }
            else if (object_group->_name == "bouncers")
            {
               auto bouncer = std::make_shared<Bouncer>(
                  dynamic_cast<GameNode*>(this),
                  _world,
                  tmx_object->_x_px,
                  tmx_object->_y_px,
                  tmx_object->_width_px,
                  tmx_object->_height_px
               );

               bouncer->setZ(object_group->_z_index);

               _mechanism_bouncers.push_back(bouncer);

               addDebugRect(
                  bouncer->getBody(),
                  tmx_object->_x_px,
                  tmx_object->_y_px,
                  tmx_object->_width_px,
                  tmx_object->_height_px
               );
            }
            else if (object_group->_name == "conveyorbelts")
            {
               auto belt = std::make_shared<ConveyorBelt>(
                  dynamic_cast<GameNode*>(this),
                  _world,
                  tmx_object,
                  path
               );

               _mechanism_conveyor_belts.push_back(belt);

               addDebugRect(
                  belt->getBody(),
                  tmx_object->_x_px,
                  tmx_object->_y_px,
                  tmx_object->_width_px,
                  tmx_object->_height_px
               );
            }
            else if (object_group->_name == "crushers")
            {
               auto crusher = std::make_shared<Crusher>(dynamic_cast<GameNode*>(this));
               crusher->setup(tmx_object, _world);
               _mechanism_crushers.push_back(crusher);
            }
            else if (object_group->_name == "rooms")
            {
               Room::deserialize(tmx_object, _rooms);
            }
            else if (object_group->_name == "platform_paths")
            {
               if (tmx_object->_polyline)
               {
                  MovingPlatform::link(_mechanism_platforms, tmx_object);
               }
            }
            else if (object_group->_name == "weather")
            {
               sf::IntRect rect{
                  static_cast<int32_t>(tmx_object->_x_px),
                  static_cast<int32_t>(tmx_object->_y_px),
                  static_cast<int32_t>(tmx_object->_width_px),
                  static_cast<int32_t>(tmx_object->_height_px)
               };

               if (tmx_object->_name.rfind("rain", 0) == 0)
               {
                  Weather::getInstance().add(Weather::WeatherType::Rain, rect);
               }
            }
            else if (object_group->_name.rfind("shader_quads", 0) == 0)
            {
               auto quad = ShaderLayer::deserialize(tmx_object);
               _shader_layers.push_back(quad);
            }
            else if (object_group->_name == "lights")
            {
               auto light = LightSystem::createLightInstance(tmx_object);
               _light_system->_lights.push_back(light);
            }
            else if (object_group->_name.compare(0, StaticLight::sLayerName.size(), StaticLight::sLayerName) == 0)
            {
               auto light = StaticLight::deserialize(tmx_object, object_group);
               _static_light->mLights.push_back(light);
            }
            if (object_group->_name == "switchable_objects")
            {
               Lever::addSearchRect(tmx_object);
            }
         }
      }

      else if (element->_type == TmxElement::TypeImageLayer)
      {
         auto image = ImageLayer::deserialize(element, path);
         _image_layers.push_back(image);
      }
   }

   Laser::merge();
   Fan::merge();
   _mechanism_fans = Fan::getFans();
   Lever::merge(_mechanism_levers, _mechanism_lasers, _mechanism_platforms, _mechanism_fans, _mechanism_conveyor_belts, _mechanism_spikes);

   _map->loadLevelTextures(
      path / std::filesystem::path("physics_grid_solid.png"),
      path / std::filesystem::path("physics_path_solid.png")
   );

   _map->setDoors(_mechanism_doors);
   _map->setPortals(_mechanism_portals);

   if (!_atmosphere.mTileMap)
   {
      std::cerr << "[E] fatal: no physics layer (called 'physics') found!" << std::endl;
   }

   std::cout << "[x] loading tmx, done within " << elapsed.getElapsedTime().asSeconds() << "s" << std::endl;
}


//-----------------------------------------------------------------------------
BoomEffect& Level::getBoomEffect()
{
    return _boom_effect;
}


//-----------------------------------------------------------------------------
void Level::load()
{
   auto path = std::filesystem::path(_description->_filename).parent_path();

   Weather::getInstance().clear();
   Checkpoint::resetAll();
   Dialogue::resetAll();

   // load tmx
   loadTmx();

   // load static lights
   std::cout << "[x] loading static lights..." << std::endl;
   if (!_static_light->mLights.empty())
   {
      _static_light->load();
   }

   // loading ao
   std::cout << "[x] loading ao... " << std::endl;
   mAo.load(path, std::filesystem::path(_description->_filename).stem().string());

   std::cout << "[x] level loading complete" << std::endl;
}


//-----------------------------------------------------------------------------
void Level::initialize()
{
   createViews();

   _description = LevelDescription::load(_description_filename);

   load();

   _start_position.x = static_cast<float_t>(_description->_start_position.at(0) * PIXELS_PER_TILE  + PLAYER_ACTUAL_WIDTH / 2);
   _start_position.y = static_cast<float_t>(_description->_start_position.at(1) * PIXELS_PER_TILE + DIFF_PLAYER_TILE_TO_PHYSICS);

   loadCheckpoint();

   spawnEnemies();
}


//-----------------------------------------------------------------------------
void Level::loadCheckpoint()
{
   auto checkpoint_index = SaveState::getCurrent().mCheckpoint;
   auto checkpoint = Checkpoint::getCheckpoint(checkpoint_index);

   if (checkpoint)
   {
      auto pos = checkpoint->calcCenter();
      _start_position.x = static_cast<float>(pos.x);
      _start_position.y = static_cast<float>(pos.y);
      std::cout << "[-] move to checkpoint: " << checkpoint_index << std::endl;
   }
   else
   {
      std::cerr << "[!] level doesn't have a start check point set up, falling back to start position" << std::endl;
   }
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
void Level::resetDeathShader()
{
   _death_shader->reset();
}


//-----------------------------------------------------------------------------
void Level::spawnEnemies()
{
   // deprecated approach:
   // merge enemy layer from tmx with enemy info that's stored inside json
   // iterate through all enemies in the json
   for (auto& json_description : _description->_enemies)
   {
      auto lua_node = LuaInterface::instance()->addObject(std::string("data/scripts/enemies/") + json_description.mScript);

      // find matching enemy data from the tmx layer and retrieve the patrol path from there
      const auto& it = _enemy_data_from_tmx_layer.find(json_description.mId);
      if (it != _enemy_data_from_tmx_layer.end())
      {
         // positions from the tmx are given in pixels, not tiles
         json_description.mPositionGivenInTiles = false;

         json_description.mStartPosition.push_back(it->second._pixel_position.x);
         json_description.mStartPosition.push_back(it->second._pixel_position.y);

         if (json_description.mGeneratePatrolPath)
         {
            it->second.addPaths(_world_chains);
         }

         if (!it->second._pixel_path.empty())
         {
            json_description.mPath = it->second._pixel_path;
         }

         // merge properties from tmx with those loaded from json
         for (auto& property : it->second._properties)
         {
            json_description.mProperties.push_back(property);
         }
      }

      // initialize lua node and store enemy
      lua_node->mEnemyDescription = json_description;
      lua_node->initialize();
      _enemies.push_back(lua_node);
   }

   // those enemies that have a lua script associated inside the tmx layer don't need
   // additional information from json, those can just be spawned.
   // this should probably be the future and only approach how to handle enemy spawning.
   for (auto& it : _enemy_data_from_tmx_layer)
   {
      auto script = it.second.findProperty("script");

      if (script.has_value())
      {
         auto lua_node = LuaInterface::instance()->addObject(std::string("data/scripts/enemies/") + script.value().mValue);

         EnemyDescription json_description;
         json_description.mPositionGivenInTiles = false;
         json_description.mStartPosition.push_back(it.second._pixel_position.x);
         json_description.mStartPosition.push_back(it.second._pixel_position.y);

         if (json_description.mGeneratePatrolPath)
         {
            it.second.addPaths(_world_chains);
         }

         if (!it.second._pixel_path.empty())
         {
            json_description.mPath = it.second._pixel_path;
         }

         // merge properties from tmx with those loaded from json
         for (auto& property : it.second._properties)
         {
            json_description.mProperties.push_back(property);
         }

         // initialize lua node and store enemy
         lua_node->mEnemyDescription = json_description;
         lua_node->initialize();
         _enemies.push_back(lua_node);
      }
   }
}


//-----------------------------------------------------------------------------
void Level::drawStaticChains(sf::RenderTarget& target)
{
   for (auto& path : _atmosphere.mOutlines)
   {
      target.draw(&path.at(0), path.size(), sf::LineStrip);
   }
}


//-----------------------------------------------------------------------------
std::shared_ptr<sf::View> Level::getLevelView()
{
  return _level_view;
}


//-----------------------------------------------------------------------------
void Level::createViews()
{
   auto& gameConfig = GameConfiguration::getInstance();

   // the view dimensions never change
   _view_width = static_cast<float>(gameConfig._view_width);
   _view_height = static_cast<float>(gameConfig._view_height);

   _level_view.reset();
   _level_view = std::make_shared<sf::View>();
   _level_view->reset(sf::FloatRect(0.0f, 0.0f, _view_width, _view_height));
   _level_view->setViewport(sf::FloatRect(0.0f, 0.0f, 1.0f, 1.0f));

   for (auto i = 0; i < 3; i++)
   {
      _parallax_view[i].reset();
      _parallax_view[i] = std::make_shared<sf::View>();
      _parallax_view[i]->reset(sf::FloatRect(0.0f, 0.0f, _view_width, _view_height));
      _parallax_view[i]->setViewport(sf::FloatRect(0.0f, 0.0f, 1.0f, 1.0f));
   }
}


//-----------------------------------------------------------------------------
void Level::updateViews()
{
   const auto lookVector = CameraPane::getInstance().getLookVector();

   const auto& cameraSystem = CameraSystem::getCameraSystem();

   const auto levelViewX = cameraSystem.getX() + lookVector.x;
   const auto levelViewY = cameraSystem.getY() + lookVector.y;

   _level_view->reset(
      sf::FloatRect(
         levelViewX,
         levelViewY,
         _view_width,
         _view_height
      )
   );

   for (auto i = 0; i < 3; i++)
   {
      _parallax_view[i]->reset(
         sf::FloatRect(
            levelViewX * _parallax_factors[i],
            levelViewY * _parallax_factors[i],
            _view_width,
            _view_height
         )
      );
   }
}


//-----------------------------------------------------------------------------
void Level::updateCameraSystem(const sf::Time& dt)
{
   auto& cameraSystem = CameraSystem::getCameraSystem();

   // update room
   const auto prev_room = _room_current;
   _room_current = Room::find(Player::getCurrent()->getPixelPositionf(), _rooms);

   // room changed
   if (prev_room != _room_current)
   {
      std::cout
         << "[i] player moved to room: "
         << (_room_current ? _room_current->_name : "undefined")
         << std::endl;

      // will update the current room in both cases, either after the camera lock delay or instantly
      if (_room_current && _room_current->_camera_lock_delay.has_value())
      {
         _room_current->lockCamera();
      }
      else
      {
         cameraSystem.setRoom(_room_current);
      }

      // trigger transition effect here if configured
      if (_room_current && _room_current->_transition_effect.has_value())
      {
         _room_current->startTransition();
      }
   }

   // update camera system
   if (!_room_current || (_room_current && !_room_current->_camera_locked))
   {
      cameraSystem.update(dt, _view_width, _view_height);
   }
}


//-----------------------------------------------------------------------------
void Level::drawNormalMap()
{
   auto tile_maps = _tile_maps;

   std::sort(tile_maps.begin(), tile_maps.end(), []( const auto& lhs, const auto& rhs)
   {
      return lhs->getZ() < rhs->getZ();
   });

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
   //      mLightingTexture->getTexture().copyToImage().saveToFile("light_map.png");
   //   }
}


//-----------------------------------------------------------------------------
void Level::drawLightAndShadows(sf::RenderTarget& target)
{
   target.setView(*_level_view);
   _light_system->draw(target, {});
}


//-----------------------------------------------------------------------------
void Level::drawParallaxMaps(sf::RenderTarget& target)
{
  for (auto i = 0u; i < _parallax_maps.size(); i++)
  {
     target.setView(*_parallax_view[i]);
     target.draw(*_parallax_maps[i]);
  }
}


//-----------------------------------------------------------------------------
void Level::drawPlayer(sf::RenderTarget& color, sf::RenderTarget& normal)
{
   auto player = Player::getCurrent();

   if (player->isDead())
   {
      auto death_render_texture = _death_shader->getRenderTexture();

      // render player to texture
      death_render_texture->clear(sf::Color{0, 0, 0, 0});
      death_render_texture->setView(*_level_view);
      player->draw(*death_render_texture, normal);
      death_render_texture->display();

      // render texture with shader applied
      auto deathShaderSprite = sf::Sprite(death_render_texture->getTexture());

      // TODO: have a static view for rendertexture quads
      sf::View view(
         sf::FloatRect(
            0.0f,
            0.0f,
            static_cast<float>(_render_texture_level->getSize().x),
            static_cast<float>(_render_texture_level->getSize().y)
         )
      );

      view.setViewport(sf::FloatRect(0.0f, 0.0f, 1.0f, 1.0f));
      color.setView(view);
      color.draw(deathShaderSprite, &_death_shader->getShader());

      takeScreenshot("screenshot_death_anim", *_death_shader->getRenderTexture());

      color.setView(*_level_view);
   }
   else
   {
      player->draw(color, normal);
   }
}


void Level::drawLayers(
   sf::RenderTarget& target,
   sf::RenderTarget& normal,
   int32_t from,
   int32_t to
)
{
   target.setView(*_level_view);
   normal.setView(*_level_view);

   for (auto z = from; z <= to; z++)
   {
      _static_light->drawToZ(target, {}, z);

      for (const auto& smoke : _smoke_effect)
      {
         smoke->drawToZ(target, {}, z);
      }

      // TODO: it's not expected that tiles are in different z layers
      //       and then unify them in one big loop

      for (auto& tileMap : _tile_maps)
      {
         if (tileMap->getZ() == z)
         {
            tileMap->draw(target, normal, {});
         }
      }

      for (auto& mechanismVector : _mechanisms)
      {
         for (auto& mechanism : *mechanismVector)
         {
            if (mechanism->getZ() == z)
            {
               mechanism->draw(target, *_render_texture_normal.get());
            }
         }
      }

      // enemies
      for (auto& enemy : _enemies)
      {
         if (enemy->mZ == z)
         {
            enemy->draw(target);
         }
      }

      if (z == ZDepthPlayer)
      {
         // ambient occlusion
         mAo.draw(target);

         // draw player
         drawPlayer(target, *_render_texture_normal.get());
      }

      for (auto& layer : _image_layers)
      {
         if (layer->mZ == z)
         {
            target.draw(layer->mSprite, {layer->mBlendMode});
         }
      }

      for (auto& layer : _shader_layers)
      {
         if (layer->_z == z)
         {
            layer->draw(target);
         }
      }
   }
}


//-----------------------------------------------------------------------------
void Level::drawAtmosphereLayer(sf::RenderTarget& target)
{
   if (!_atmosphere.mTileMap)
   {
      return;
   }

   _atmosphere.mTileMap->setVisible(true);

   target.setView(*_level_view);
   target.draw(*_atmosphere.mTileMap);

   _atmosphere.mTileMap->setVisible(false);
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
bool Level::isPhysicsPathClear(const sf::Vector2i& a, const sf::Vector2i& b) const
{
   auto blocks = [this](uint32_t x, uint32_t y) -> bool {
      return _physics.mPhysicsMap[(_physics.mGridWidth * y) + x] == 1;
   };

   return MapTools::lineCollide(a.x, a.y, b.x, b.y, blocks);
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
   if (DisplayMode::getInstance().isSet(Display::DisplayDebug))
   {
      drawStaticChains(*_render_texture_level.get());
      DebugDraw::debugBodies(*_render_texture_level.get(), this);
      DebugDraw::drawRect(*_render_texture_level.get(), Player::getCurrent()->getPlayerPixelRect());

      for (const auto& room : _rooms)
      {
         for (const auto& rect : room->_rects)
         {
            DebugDraw::drawRect(*_render_texture_level.get(), rect, sf::Color::Yellow);
         }
      }
   }
}


//-----------------------------------------------------------------------------
void Level::displayTextures()
{
   // display the whole texture
   sf::View view(
      sf::FloatRect(
         0.0f,
         0.0f,
         static_cast<float>(_render_texture_level->getSize().x),
         static_cast<float>(_render_texture_level->getSize().y)
      )
   );

   view.setViewport(sf::FloatRect(0.0f, 0.0f, 1.0f, 1.0f));

   _render_texture_level->setView(view);
   _render_texture_level->display();

   _render_texture_normal->setView(*_level_view);
   _render_texture_normal->display();
}


void Level::drawGlowLayer()
{
   #ifdef GLOW_ENABLED
      mBlurShader->clearTexture();
      drawBlurLayer(*mBlurShader->getRenderTexture().get());
      mBlurShader->getRenderTexture()->display();
      takeScreenshot("screenshot_blur", *mBlurShader->getRenderTexture().get());
   #endif
}


void Level::drawGlowSprite()
{
#ifdef GLOW_ENABLED
   sf::Sprite blurSprite(mBlurShader->getRenderTexture()->getTexture());
   const auto downScaleX = mBlurShader->getRenderTextureScaled()->getSize().x / static_cast<float>(mBlurShader->getRenderTexture()->getSize().x);
   const auto downScaleY = mBlurShader->getRenderTextureScaled()->getSize().y / static_cast<float>(mBlurShader->getRenderTexture()->getSize().y);
   blurSprite.scale({downScaleX, downScaleY});

   sf::RenderStates statesShader;
   mBlurShader->update();
   statesShader.shader = &mBlurShader->getShader();
   mBlurShader->getRenderTextureScaled()->draw(blurSprite, statesShader);

   sf::Sprite blurScaleSprite(mBlurShader->getRenderTextureScaled()->getTexture());
   blurScaleSprite.scale(1.0f / downScaleX, 1.0f / downScaleY);
   blurScaleSprite.setTextureRect(
      sf::IntRect(
         0,
         static_cast<int32_t>(blurScaleSprite.getTexture()->getSize().y),
         static_cast<int32_t>(blurScaleSprite.getTexture()->getSize().x),
         -static_cast<int32_t>(blurScaleSprite.getTexture()->getSize().y)
      )
   );

   sf::RenderStates statesAdd;
   statesAdd.blendMode = sf::BlendAdd;
   mLevelRenderTexture->draw(blurScaleSprite, statesAdd);
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
void Level::draw(
   const std::shared_ptr<sf::RenderTexture>& window,
   bool screenshot
)
{
   _screenshot = screenshot;

   // render atmosphere to atmosphere texture, that texture is used in the shader only
   _atmosphere_shader->getRenderTexture()->clear();
   drawAtmosphereLayer(*_atmosphere_shader->getRenderTexture().get());
   _atmosphere_shader->getRenderTexture()->display();
   takeScreenshot("screenshot_atmosphere", *_atmosphere_shader->getRenderTexture().get());

   // render glowing elements
   drawGlowLayer();

   // render layers affected by the atmosphere
   _render_texture_level_background->clear();
   _render_texture_normal->clear();

   drawParallaxMaps(*_render_texture_level_background.get());
   drawLayers(
      *_render_texture_level_background.get(),
      *_render_texture_normal.get(),
      ZDepthBackgroundMin,
      ZDepthBackgroundMax
   );
   _render_texture_level_background->display();
   takeScreenshot("screenshot_level_background", *_render_texture_level_background.get());

   // draw the atmospheric parts into the level texture
   sf::Sprite backgroundSprite(_render_texture_level_background->getTexture());
   _atmosphere_shader->update();
   _render_texture_level->draw(backgroundSprite, &_atmosphere_shader->getShader());

   drawGlowSprite();

   // draw the level layers into the level texture
   drawLayers(
      *_render_texture_level.get(),
      *_render_texture_normal.get(),
      ZDepthForegroundMin,
      ZDepthForegroundMax
   );

   Weapon::drawProjectileHitAnimations(*_render_texture_level.get());
   AnimationPlayer::getInstance().draw(*_render_texture_level.get());

   drawDebugInformation();

   displayTextures();

   drawLightMap();

   _light_system->draw(
      *_render_texture_deferred.get(),
      _render_texture_level,
      _render_texture_lighting,
      _render_texture_normal
   );

   _render_texture_deferred->display();

   takeScreenshot("map_color",    *_render_texture_level.get());
   takeScreenshot("map_light",    *_render_texture_lighting.get());
   takeScreenshot("map_normal",   *_render_texture_normal.get());
   takeScreenshot("map_deferred", *_render_texture_deferred.get());

   auto levelTextureSprite = sf::Sprite(_render_texture_deferred->getTexture());
   _gamme_shader->setTexture(_render_texture_deferred->getTexture());

   levelTextureSprite.setPosition(_boom_effect.mBoomOffsetX, _boom_effect.mBoomOffsetY);
   levelTextureSprite.scale(_view_to_texture_scale, _view_to_texture_scale);

   _gamme_shader->update();
   window->draw(levelTextureSprite, &_gamme_shader->getGammaShader());

   if (DisplayMode::getInstance().isSet(Display::DisplayMap))
   {
      _map->draw(*window.get());
   }
}


//-----------------------------------------------------------------------------
void Level::updatePlayerLight()
{
   _player_light->_pos_m = Player::getCurrent()->getBody()->GetPosition();
   _player_light->updateSpritePosition();

   // the player, once he dies, becomes inactive and just sinks down
   // so the player light is disabled to avoid any glitches
   _player_light->_color = sf::Color(255, 255, 255, Player::getCurrent()->isDead()? 0 : 10);
}


//-----------------------------------------------------------------------------
std::shared_ptr<LightSystem> Level::getLightSystem() const
{
   return _light_system;
}


//-----------------------------------------------------------------------------
void Level::update(const sf::Time& dt)
{
   // clear conveyor belt state
   ConveyorBelt::update();

   updateCameraSystem(dt);
   updateViews();

   // 80.0f * dt / 60.f
   // http://www.iforce2d.net/b2dtut/worlds
   _world->Step(PhysicsConfiguration::getInstance().mTimeStep, 8, 3);

   CameraPane::getInstance().update();
   _boom_effect.update(dt);

   Checkpoint::update();
   Dialogue::update();

   AnimationPlayer::getInstance().update(dt);

   for (auto& tileMap : _tile_maps)
   {
      tileMap->update(dt);
   }

   for (auto mechanismVector : _mechanisms)
   {
      for (auto& mechanism : *mechanismVector)
      {
         mechanism->update(dt);
      }
   }

   LuaInterface::instance()->update(dt);

   updatePlayerLight();

   _static_light->update(GlobalClock::getInstance()->getElapsedTime(), 0.0f, 0.0f);

   for (const auto& smoke : _smoke_effect)
   {
      smoke->update(GlobalClock::getInstance()->getElapsedTime(), 0.0f, 0.0f);
   }

   _death_shader->update(dt);
}


//-----------------------------------------------------------------------------
const std::shared_ptr<b2World>& Level::getWorld() const
{
   return _world;
}


//-----------------------------------------------------------------------------
void Level::addChainToWorld(
   const std::vector<b2Vec2>& chain,
   ObjectType object_type
)
{
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

   b2Body* body = _world->CreateBody(&body_def);

   auto fixture = body->CreateFixture(&fixture_def);

   auto object_data = new FixtureNode(this);
   object_data->setType(object_type);

   fixture->SetUserData(static_cast<void*>(object_data));
}


//-----------------------------------------------------------------------------
void Level::addDebugOutlines(
   int32_t offsetX,
   int32_t offsetY,
   std::vector<sf::Vector2f> positions,
   ObjectType behavior
)
{
   sf::Color color;
   switch (behavior)
   {
      case ObjectTypeSolid:
         color = sf::Color(255, 255, 255);
         break;
      case ObjectTypeDeadly:
         color = sf::Color(255, 0, 0);
         break;
      case ObjectTypeSolidOneSided:
         color = sf::Color(255, 255, 0);
         break;
      default:
         break;
   }

   // path.printPoly();
   std::vector<sf::Vertex> visible_path;
   for (auto& pos : positions)
   {
      sf::Vertex visibleVertex;
      visibleVertex.color = color;
      visibleVertex.position.x = static_cast<float_t>((pos.x + offsetX) * PIXELS_PER_TILE);
      visibleVertex.position.y = static_cast<float_t>((pos.y + offsetY) * PIXELS_PER_TILE);

      visible_path.push_back(visibleVertex);
   }

   visible_path.push_back(visible_path.at(0));
   _atmosphere.mOutlines.push_back(visible_path);
}


//-----------------------------------------------------------------------------
void Level::addPathsToWorld(
   int32_t offsetX,
   int32_t offsetY,
   const std::vector<SquareMarcher::Path>& paths,
   ObjectType behavior
)
{
   // just for debugging purposes, this section can be removed later
   // for (auto& path : paths)
   // {
   //    const auto& scaled = path.mScaled;
   //    addDebugOutlines(offsetX, offsetY, scaled, behavior);
   // }

   // create the physical chain with 1 body per chain
   for (auto& path : paths)
   {
      std::vector<b2Vec2> chain;
      for (auto& pos : path.mScaled)
      {
         chain.push_back({
               (pos.x + offsetX) * PIXELS_PER_TILE / PPM,
               (pos.y + offsetY) * PIXELS_PER_TILE / PPM
            }
         );
      }

      addChainToWorld(chain, behavior);
   }
}


//-----------------------------------------------------------------------------
void Level::parseObj(
   TmxLayer* layer,
   ObjectType behavior,
   const std::filesystem::path& path
)
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
         chain.push_back({
               (p.x + layer->_offset_x_px) / PPM,
               (p.y + layer->_offset_y_px) / PPM
            }
         );

         debug_path.push_back({
               p.x / PIXELS_PER_TILE,
               p.y / PIXELS_PER_TILE
            }
         );
      }

      // creating a box2d chain is automatically closing the path
      chain.pop_back();

      addChainToWorld(chain, behavior);

      // not required any longer
      // addDebugOutlines(layer->mOffsetX, layer->mOffsetY, debugPath, behavior);

      // Mesh::writeVerticesToImage(points, faces, {1200, 1200}, "yo_yo.png");
   }
}


//-----------------------------------------------------------------------------
void Level::parsePhysicsTiles(
   TmxLayer* layer,
   TmxTileSet* tileset,
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
   solid_onesided_pd.object_type = ObjectTypeSolidOneSided;
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

   static const float scale = 1.0f / 3.0f;

   auto path_solid_optimized = base_path / std::filesystem::path(pd->filename_obj_optimized);

   std::cout << "[x] loading: " << path_solid_optimized.make_preferred().generic_string() << std::endl;

   _physics.parse(layer, tileset, base_path);

   // this whole block should be generated by an external tool
   // right now the squaremarcher output is still used for the in-game map visualization
   SquareMarcher square_marcher(
      _physics.mGridWidth,
      _physics.mGridHeight,
      _physics.mPhysicsMap,
      pd->colliding_tiles,
      base_path / std::filesystem::path(pd->filename_physics_path_csv),
      scale
   );

   square_marcher.writeGridToImage(base_path / std::filesystem::path(pd->filename_grid_image)); // not needed
   square_marcher.writePathToImage(base_path / std::filesystem::path(pd->filename_path_image)); // needed from obj as well

   if (std::filesystem::exists(path_solid_optimized))
   {
      parseObj(layer, pd->object_type, path_solid_optimized);
   }
   else
   {
      const auto pathSolidNotOptimised = base_path / std::filesystem::path(pd->filename_obj_not_optimized);

      // dump the tileset into an obj file, optimise that and load it
      if (_physics.dumpObj(layer, tileset, pathSolidNotOptimised))
      {
#ifdef __linux__
          auto cmd = std::string("tools/path_merge/path_merge") + " "
                + pathSolidNotOptimised.string() + " "
                + pathSolidOptimized.string();
#else
          auto cmd = std::string("tools\\path_merge\\path_merge.exe") + " "
                + pathSolidNotOptimised.string() + " "
                + path_solid_optimized.string();
#endif

         std::cout << "[x] running cmd: " << cmd << std::endl;

         if (std::system(cmd.c_str()) != 0)
         {
            std::cerr << "[!] command failed" << std::endl;
         }
         else
         {
            std::cout << "[x] command succeeded" << std::endl;
         }
      }
      else
      {
         std::cerr << "[!] dumping unoptimized obj (" << pathSolidNotOptimised<< ") failed" << std::endl;
      }

      // fallback to square marched level
      if (!std::filesystem::exists(path_solid_optimized))
      {
         std::cerr << "[!] could not find " << path_solid_optimized.string() << ", obj generator failed" << std::endl;
         addPathsToWorld(layer->_offset_x_px, layer->_offset_y_px, square_marcher.mPaths, pd->object_type);
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
//         mPhysics.mGridWidth,
//         mPhysics.mGridHeight,
//         mPhysics.mPhysicsMap,
//         std::vector<int32_t>{3},
//         basePath / std::filesystem::path("physics_path_deadly.csv"),
//         scale
//      );
//
//      addPathsToWorld(layer->mOffsetX, layer->mOffsetY, deadly.mPaths, ObjectTypeDeadly);
//   }
}


//-----------------------------------------------------------------------------
const sf::Vector2f &Level::getStartPosition() const
{
   return _start_position;
}


//-----------------------------------------------------------------------------
Level* Level::getCurrentLevel()
{
   return __current_level;
}


//-----------------------------------------------------------------------------
void Level::addDebugRect(b2Body* body,  float x, float y, float w, float h)
{
   auto points = new b2Vec2[4];

   points[0] = b2Vec2(x * MPP,           y * MPP          );
   points[1] = b2Vec2(x * MPP + w * MPP, y * MPP          );
   points[2] = b2Vec2(x * MPP + w * MPP, y * MPP + h * MPP);
   points[3] = b2Vec2(x * MPP,           y * MPP + h * MPP);

   _point_map[body] = points;
   _point_count_map[body] = 4;
}


//-----------------------------------------------------------------------------
AtmosphereTile Atmosphere::getTileForPosition(const b2Vec2& pos) const
{
   auto x = pos.x - mMapOffsetX;
   auto y = pos.y - mMapOffsetY;

   if (x < 0 || x >= mMapWidth)
   {
      return AtmosphereTileInvalid;
   }

   if (y < 0 || y >= mMapHeight)
   {
      return AtmosphereTileInvalid;
   }

   auto tx = static_cast<uint32_t>(x * PPM / PIXELS_PER_TILE);
   auto ty = static_cast<uint32_t>(y * PPM / PIXELS_PER_TILE);

   AtmosphereTile tile = static_cast<AtmosphereTile>(mMap[ty * mMapWidth + tx]);
   return tile;
}


//-----------------------------------------------------------------------------
std::shared_ptr<Portal> Level::getNearbyPortal()
{
   std::shared_ptr<Portal> nearbyPortal;

   for (auto& p : _mechanism_portals)
   {
      auto portal = std::dynamic_pointer_cast<Portal>(p);
      if (portal->isPlayerAtPortal())
      {
         nearbyPortal = portal;
         break;
      }
   }

   return nearbyPortal;
}


//-----------------------------------------------------------------------------
std::shared_ptr<Bouncer> Level::getNearbyBouncer()
{
   std::shared_ptr<Bouncer> nearbyBouncer;

   for (auto& tmp : _mechanism_bouncers)
   {
      auto bouncer = std::dynamic_pointer_cast<Bouncer>(tmp);
      if (bouncer->isPlayerAtBouncer())
      {
         nearbyBouncer = bouncer;
         break;
      }
   }

   return nearbyBouncer;
}


//-----------------------------------------------------------------------------
void Level::toggleMechanisms()
{
   for (auto& door : _mechanism_doors)
   {
      std::dynamic_pointer_cast<Door>(door)->toggle();
   }

   for (auto& lever : _mechanism_levers)
   {
      std::dynamic_pointer_cast<Lever>(lever)->toggle();
   }
}


//-----------------------------------------------------------------------------
std::map<b2Body*, size_t>* Level::getPointSizeMap()
{
   return &_point_count_map;
}


//-----------------------------------------------------------------------------
std::map<b2Body *, b2Vec2 *>* Level::getPointMap()
{
   return &_point_map;
}


