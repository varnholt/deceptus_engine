#include "crusher.h"

#include "fixturenode.h"

#include "framework/easings/easings.h"

#include "framework/tmxparser/tmximage.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxpolyline.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxtileset.h"
#include "texturepool.h"

#include <iostream>

int32_t Crusher::__instance_counter = 0;

namespace
{
static constexpr auto BLADE_HORIZONTAL_TILES = 5;
static constexpr auto BLADE_VERTICAL_TILES = 1;

static constexpr auto BLADE_SIZE_X = (BLADE_HORIZONTAL_TILES * PIXELS_PER_TILE) / PPM;
static constexpr auto BLADE_SIZE_Y = (BLADE_VERTICAL_TILES * PIXELS_PER_TILE) / PPM;

static constexpr auto BLADE_SHARPNESS = 0.1f;
static constexpr auto BLADE_TOLERANCE = 0.06f;
}


//-----------------------------------------------------------------------------
Crusher::Crusher(GameNode* parent)
   : GameNode(parent)
{
   setName("DeathBlock");

   _texture = TexturePool::getInstance().get("data/level-crypt/tilesets/crushers.png");

   _instance_id = __instance_counter;
   __instance_counter++;
}


//-----------------------------------------------------------------------------
void Crusher::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   color.draw(_sprite_spike);
   color.draw(_sprite_pusher);
   color.draw(_sprite_mount);
}


//-----------------------------------------------------------------------------
void Crusher::step(const sf::Time& dt)
{
   const auto distance_to_be_traveled = 48.0f;

   switch (_state)
   {
      case State::Idle:
      {
         _idle_time += dt;
         break;
      }
      case State::Extract:
      {
         const auto val = distance_to_be_traveled * Easings::easeOutBounce<float>(_extraction_time.asSeconds());

         switch (_alignment)
         {
            case Alignment::PointsDown:
               _blade_offset.y = val;
               break;
            case Alignment::PointsUp:
               _blade_offset.y = -val;
               break;
            case Alignment::PointsLeft:
               _blade_offset.x = -val;
               break;
            case Alignment::PointsRight:
               _blade_offset.x = val;
               break;
            case Alignment::PointsNowhere:
               break;
         }


         _extraction_time += dt * 1.0f;

         break;
      }
      case State::Retract:
      {
         const auto val = distance_to_be_traveled * (1.0f - Easings::easeInSine<float>(_retraction_time.asSeconds()));

         switch (_alignment)
         {
            case Alignment::PointsDown:
               _blade_offset.y = val;
               break;
            case Alignment::PointsUp:
               _blade_offset.y = -val;
               break;
            case Alignment::PointsLeft:
               _blade_offset.x = -val;
               break;
            case Alignment::PointsRight:
               _blade_offset.x = val;
               break;
            case Alignment::PointsNowhere:
               break;
         }

         _retraction_time += dt * 0.4f;

         break;
      }
   }
}


void Crusher::update(const sf::Time& dt)
{
   updateState();
   step(dt);
   updateSpritePositions();
   updateTransform();
}


//-----------------------------------------------------------------------------
void Crusher::updateState()
{
   switch (_mode)
   {
      case Mode::Interval:
      {
         switch (_state)
         {
            case State::Idle:
            {
               // go to extract when idle time is elapsed
               if (_idle_time.asSeconds() > 3.0f)
               {
                  _idle_time = {};

                  if (_state_previous == State::Retract || _state_previous == State::Idle)
                  {
                     _state = State::Extract;
                  }
                  else
                  {
                     _state = State::Retract;
                  }
               }
               break;
            }
            case State::Extract:
            {
               // extract until normalised extraction time is 1
               if (_extraction_time.asSeconds() >= 1.0f)
               {
                  _state = State::Idle;
                  _state_previous = State::Extract;
                  _extraction_time = {};
               }
               break;
            }
            case State::Retract:
            {
               // retract until normalised retraction time is 1
               if (_retraction_time.asSeconds() >= 1.0f)
               {
                  _state = State::Idle;
                  _state_previous = State::Retract;
                  _retraction_time = {};
               }
               break;
            }
         }

         break;
      }
      case Mode::Distance:
      {
         break;
      }
   }
}


//-----------------------------------------------------------------------------
void Crusher::setup(TmxObject* tmxObject, const std::shared_ptr<b2World>& world)
{
   if (tmxObject->_properties)
   {
      const auto it = tmxObject->_properties->_map.find("alignment");

      if (it != tmxObject->_properties->_map.end())
      {
         // std::cout << it->second->mValueStr << std::endl;

         const auto alignment = it->second->_value_string;
         if (alignment == "up")
         {
            _alignment = Alignment::PointsUp;
         }
         else if (alignment == "down")
         {
            _alignment = Alignment::PointsDown;
         }
         else if (alignment == "left")
         {
            _alignment = Alignment::PointsLeft;
         }
         else if (alignment == "right")
         {
            _alignment = Alignment::PointsRight;
         }
      }
   }

   //    0123456789012
   //   +-------------+
   //  0|<#         #>|
   //  1|<#         #>|
   //  2|<#CBAA AABC#>|
   //  3|<#         #>|
   //  4|<#         #>|
   //  6|^^^^^    A   |
   //  7|#####    A   |
   //  8|  C      B   |
   //  9|  B      C   |
   //  0|  A    ##### |
   //  1|  A    VVVVV |
   //   +-------------+
   //    0123456789012

   _pixel_position.x = tmxObject->_x_px;
   _pixel_position.y = tmxObject->_y_px;

   _sprite_mount.setTexture(*_texture);
   _sprite_pusher.setTexture(*_texture);
   _sprite_spike.setTexture(*_texture);

   switch (_alignment)
   {
      case Alignment::PointsDown:
      {
         _sprite_mount.setTextureRect({
               7 * PIXELS_PER_TILE,
               5 * PIXELS_PER_TILE,
               5 * PIXELS_PER_TILE,
               2 * PIXELS_PER_TILE
            }
         );

         _sprite_pusher.setTextureRect({
               7 * PIXELS_PER_TILE,
               7 * PIXELS_PER_TILE,
               5 * PIXELS_PER_TILE,
               1 // * PIXELS_PER_TILE - i only want this to be one pixel in height so scaling is easy
            }
         );

         _sprite_spike.setTextureRect({
               7 * PIXELS_PER_TILE,
               8 * PIXELS_PER_TILE,
               5 * PIXELS_PER_TILE,
               3 * PIXELS_PER_TILE
            }
         );

         _pixel_offset_pusher.y = 2 * PIXELS_PER_TILE;
         _pixel_offset_spike.y = 2 * PIXELS_PER_TILE;

         break;
      }

      case Alignment::PointsUp:
      {
         _sprite_mount.setTextureRect({
               0 * PIXELS_PER_TILE,
               9 * PIXELS_PER_TILE,
               5 * PIXELS_PER_TILE,
               2 * PIXELS_PER_TILE
            }
         );

         _sprite_pusher.setTextureRect({
               0 * PIXELS_PER_TILE,
               8 * PIXELS_PER_TILE,
               5 * PIXELS_PER_TILE,
               1
            }
         );

         _sprite_spike.setTextureRect({
               0 * PIXELS_PER_TILE,
               5 * PIXELS_PER_TILE,
               5 * PIXELS_PER_TILE,
               3 * PIXELS_PER_TILE
            }
         );

         _pixel_offset_pusher.y = 6 * PIXELS_PER_TILE;
         _pixel_offset_spike.y = 3 * PIXELS_PER_TILE;
         _pixel_offset_mount.y = 6 * PIXELS_PER_TILE;

         break;
      }

      case Alignment::PointsLeft:
      {
         _sprite_pusher.setTextureRect({
               3 * PIXELS_PER_TILE,
               0 * PIXELS_PER_TILE,
               1,
               5 * PIXELS_PER_TILE
            }
         );

         _sprite_mount.setTextureRect({
               4 * PIXELS_PER_TILE,
               0 * PIXELS_PER_TILE,
               2 * PIXELS_PER_TILE,
               5 * PIXELS_PER_TILE
            }
         );

         _sprite_spike.setTextureRect({
               0 * PIXELS_PER_TILE,
               0 * PIXELS_PER_TILE,
               3 * PIXELS_PER_TILE,
               5 * PIXELS_PER_TILE
            }
         );

         _pixel_offset_pusher.y = -1 * PIXELS_PER_TILE;
         _pixel_offset_pusher.x = 3 * PIXELS_PER_TILE;
         _pixel_offset_spike.y = -1 * PIXELS_PER_TILE;
         _pixel_offset_mount.y = -1 * PIXELS_PER_TILE;
         _pixel_offset_mount.x = 3 * PIXELS_PER_TILE;

         break;
      }

      case Alignment::PointsRight:
      {
         _sprite_mount.setTextureRect({
               7 * PIXELS_PER_TILE,
               0 * PIXELS_PER_TILE,
               2 * PIXELS_PER_TILE,
               5 * PIXELS_PER_TILE
            }
         );

         _sprite_pusher.setTextureRect({
               9 * PIXELS_PER_TILE,
               0 * PIXELS_PER_TILE,
               1,
               5 * PIXELS_PER_TILE
            }
         );

         _sprite_spike.setTextureRect({
              10 * PIXELS_PER_TILE,
               0 * PIXELS_PER_TILE,
               3 * PIXELS_PER_TILE,
               5 * PIXELS_PER_TILE
            }
         );

         _pixel_offset_pusher.y = -1 * PIXELS_PER_TILE;
         _pixel_offset_pusher.x = -1 * PIXELS_PER_TILE;
         _pixel_offset_spike.y = -1 * PIXELS_PER_TILE;
         _pixel_offset_spike.x = -1 * PIXELS_PER_TILE;
         _pixel_offset_mount.y = -1 * PIXELS_PER_TILE;
         _pixel_offset_mount.x = -3 * PIXELS_PER_TILE;

         break;
      }

      case Alignment::PointsNowhere:
         break;
   }

   setupBody(world);
}


//-----------------------------------------------------------------------------
void Crusher::updateTransform()
{
   auto x = (_blade_offset.x + _pixel_position.x) / PPM;
   auto y = (_blade_offset.y + _pixel_position.y - PIXELS_PER_TILE) / PPM + (5 * PIXELS_PER_TILE) / PPM;
   _body->SetTransform(b2Vec2(x, y), 0.0f);
}


//-----------------------------------------------------------------------------
void Crusher::setupBody(const std::shared_ptr<b2World>& world)
{
   //       +-+
   //       | |
   //       | |
   //       | |
   //       | |
   // +-----+-+------+
   // \             /
   //  \___________/

   b2Vec2 blade_vertices[4];

   switch (_alignment)
   {
      case Alignment::PointsLeft:
      {

         blade_vertices[0] = b2Vec2(0           ,                BLADE_SHARPNESS + BLADE_TOLERANCE - BLADE_SIZE_X);
         blade_vertices[1] = b2Vec2(0           , BLADE_SIZE_X - BLADE_SHARPNESS - BLADE_TOLERANCE - BLADE_SIZE_X);
         blade_vertices[2] = b2Vec2(BLADE_SIZE_Y,                                  BLADE_TOLERANCE - BLADE_SIZE_X);
         blade_vertices[3] = b2Vec2(BLADE_SIZE_Y, BLADE_SIZE_X                   - BLADE_TOLERANCE - BLADE_SIZE_X);
         break;
      }
      case Alignment::PointsRight:
      {
         blade_vertices[0] = b2Vec2(0            + PIXELS_PER_TILE / PPM,                                  BLADE_TOLERANCE - BLADE_SIZE_X);
         blade_vertices[1] = b2Vec2(BLADE_SIZE_Y + PIXELS_PER_TILE / PPM,                BLADE_SHARPNESS + BLADE_TOLERANCE - BLADE_SIZE_X);
         blade_vertices[2] = b2Vec2(BLADE_SIZE_Y + PIXELS_PER_TILE / PPM, BLADE_SIZE_X - BLADE_SHARPNESS - BLADE_TOLERANCE - BLADE_SIZE_X);
         blade_vertices[3] = b2Vec2(0            + PIXELS_PER_TILE / PPM, BLADE_SIZE_X                   - BLADE_TOLERANCE - BLADE_SIZE_X);
         break;
      }
      case Alignment::PointsDown:
      {
         blade_vertices[0] = b2Vec2(                                 BLADE_TOLERANCE, 0           );
         blade_vertices[1] = b2Vec2(               BLADE_SHARPNESS + BLADE_TOLERANCE, BLADE_SIZE_Y);
         blade_vertices[2] = b2Vec2(BLADE_SIZE_X - BLADE_SHARPNESS - BLADE_TOLERANCE, BLADE_SIZE_Y);
         blade_vertices[3] = b2Vec2(BLADE_SIZE_X                   - BLADE_TOLERANCE, 0           );
         break;
      }
      case Alignment::PointsUp:
      {
         blade_vertices[0] = b2Vec2(                                 BLADE_TOLERANCE, BLADE_SIZE_Y - PIXELS_PER_TILE / PPM);
         blade_vertices[1] = b2Vec2(               BLADE_SHARPNESS + BLADE_TOLERANCE, 0            - PIXELS_PER_TILE / PPM);
         blade_vertices[2] = b2Vec2(BLADE_SIZE_X - BLADE_SHARPNESS - BLADE_TOLERANCE, 0            - PIXELS_PER_TILE / PPM);
         blade_vertices[3] = b2Vec2(BLADE_SIZE_X                   - BLADE_TOLERANCE, BLADE_SIZE_Y - PIXELS_PER_TILE / PPM);
         break;
      }
      case Alignment::PointsNowhere:
      {
         break;
      }
   }

   b2BodyDef deadly_body_def;
   deadly_body_def.type = b2_kinematicBody;
   _body = world->CreateBody(&deadly_body_def);

   b2PolygonShape spike_shape;
   spike_shape.Set(blade_vertices, 4);
   auto deadly_fixture = _body->CreateFixture(&spike_shape, 0);

   auto object_data = new FixtureNode(this);
   object_data->setType(ObjectTypeCrusher);
   deadly_fixture->SetUserData(static_cast<void*>(object_data));

   auto box_width = 0.0f;
   auto box_height = 0.0f;
   b2Vec2 box_center;

   switch (_alignment)
   {
      case Alignment::PointsLeft:
      {
         box_width = BLADE_SIZE_Y * 0.5f;
         box_height = BLADE_SIZE_X * 0.5f;
         box_center = {box_width, box_height};
         box_center.x += PIXELS_PER_TILE / PPM;
         box_center.y -= BLADE_SIZE_X;
         break;
      }
      case Alignment::PointsRight:
      {
         box_width = BLADE_SIZE_Y * 0.5f;
         box_height = BLADE_SIZE_X * 0.5f;
         box_center = {box_width, box_height};
         box_center.y -= BLADE_SIZE_X;
         break;
      }
      case Alignment::PointsUp:
      {
         box_width = BLADE_SIZE_X * 0.5f;
         box_height = BLADE_SIZE_Y * 0.5f;
         box_center = {box_width, box_height};
         break;
      }
      case Alignment::PointsDown:
      {
         box_width = BLADE_SIZE_X * 0.5f;
         box_height = BLADE_SIZE_Y * 0.5f;
         box_center = {box_width, box_height};
         box_center.y -= PIXELS_PER_TILE / PPM;
         break;
      }
      case Alignment::PointsNowhere:
      {
         break;
      }
   }

   b2PolygonShape box_shape;
   box_shape.SetAsBox(
      box_width,
      box_height,
      box_center,
      0.0f
   );

   _body->CreateFixture(&box_shape, 0);
}


void Crusher::updateSpritePositions()
{
   switch (_alignment)
   {
      case Alignment::PointsDown:
      {
         _sprite_pusher.setScale(1.0f, _blade_offset.y);
         break;
      }
      case Alignment::PointsUp:
      {
         _sprite_pusher.setScale(1.0f, _blade_offset.y);
         break;
      }
      case Alignment::PointsLeft:
      {
         _sprite_pusher.setScale(_blade_offset.x, 1.0f);
         break;
      }
      case Alignment::PointsRight:
      {
         _sprite_pusher.setScale(_blade_offset.x, 1.0f);
         break;
      }
      case Alignment::PointsNowhere:
      {
         break;
      }
   }

   _sprite_mount.setPosition(_pixel_position + _pixel_offset_mount);
   _sprite_pusher.setPosition(_pixel_position + _pixel_offset_pusher);
   _sprite_spike.setPosition(_pixel_position + _pixel_offset_spike + _blade_offset);
}

