#include "lever.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tools/log.h"
#include "game/audio/audio.h"
#include "game/constants.h"
#include "game/io/texturepool.h"
#include "game/player/player.h"
#include "game/state/savestate.h"

namespace
{
constexpr auto sprites_per_row = 11;
constexpr auto row_center = 6;
constexpr auto left_offset = (sprites_per_row - 1) * 3 * PIXELS_PER_TILE;
constexpr auto idle_animation_speed = 10.0f;
}  // namespace

void Lever::setup(const GameDeserializeData& data)
{
   if (data._tmx_object->_properties)
   {
      const auto z_it = data._tmx_object->_properties->_map.find("z");
      if (z_it != data._tmx_object->_properties->_map.end())
      {
         const auto z_index = static_cast<uint32_t>(z_it->second->_value_int.value());
         setZ(z_index);
      }

      const auto enabled_it = data._tmx_object->_properties->_map.find("enabled");
      if (enabled_it != data._tmx_object->_properties->_map.end())
      {
         _enabled = enabled_it->second->_value_bool.value();
         _target_state = (_enabled ? State::Right : State::Left);

         // put the lever on the correct position
         if (_enabled)
         {
            _offset = sprites_per_row - 1;
         }
      }

      const auto serialized_it = data._tmx_object->_properties->_map.find("serialized");
      if (serialized_it != data._tmx_object->_properties->_map.end())
      {
         _serialized = serialized_it->second->_value_bool.value();
      }

      const auto target_id_it = data._tmx_object->_properties->_map.find("target_id");
      if (target_id_it != data._tmx_object->_properties->_map.end())
      {
         _target_ids.push_back(target_id_it->second->_value_string.value());
      }

      const auto target_ids_it = data._tmx_object->_properties->_map.find("target_ids");
      if (target_ids_it != data._tmx_object->_properties->_map.end())
      {
         std::string tmp;
         const auto id_string = target_ids_it->second->_value_string.value();
         std::stringstream string_stream(id_string);
         while (getline(string_stream, tmp, ';'))
         {
            _target_ids.push_back(tmp);
         }
      }

      // todo: document handle_available
      const auto handle_available_it = data._tmx_object->_properties->_map.find("handle_available");
      if (handle_available_it != data._tmx_object->_properties->_map.end())
      {
         _handle_available = handle_available_it->second->_value_bool.value();

         // accept handle from inventory
         if (!_handle_available)
         {
            _handle_callback = [this](const std::string& item)
            {
               if (_player_at_lever && !_handle_available && item == "handle")
               {
                  setHandleAvailable(true);
                  Audio::getInstance().playSample(Audio::PlayInfo{"mechanism_switch_lever_insert.wav"});
                  return true;
               }
               return false;
            };

            SaveState::getPlayerInfo()._inventory._used_callbacks.push_back(_handle_callback);
         }
      }
   }

   const auto x = data._tmx_object->_x_px;
   const auto y = data._tmx_object->_y_px;

   _rect.left = x;
   _rect.top = y;
   _rect.width = PIXELS_PER_TILE * 3;
   _rect.height = PIXELS_PER_TILE * 2;

   _sprite.setPosition(x, y);
   // _texture = TexturePool::getInstance().get(data._base_path / "tilesets" / "levers.png");
   _texture = TexturePool::getInstance().get("data/sprites/levers.png");
   _sprite.setTexture(*_texture);

   setObjectId(data._tmx_object->_name);

   updateSprite();

   addChunks(_rect);
}

void Lever::updateSprite()
{
   if (_reached && (_target_state == State::Right))
   {
      _sprite.setTextureRect(
         {(static_cast<int32_t>(_idle_time_s * idle_animation_speed) % 6) * PIXELS_PER_TILE * 3,
          PIXELS_PER_TILE * 3 * 2,
          PIXELS_PER_TILE * 3,
          PIXELS_PER_TILE * 3}
      );
   }
   else
   {
      const auto left = _dir == -1;

      _sprite.setTextureRect(
         {left ? (left_offset - _offset * 3 * PIXELS_PER_TILE) : (_offset * 3 * PIXELS_PER_TILE),
          left ? (3 * PIXELS_PER_TILE) : 0,
          PIXELS_PER_TILE * 3,
          PIXELS_PER_TILE * 3}
      );
   }
}

const sf::FloatRect& Lever::getPixelRect() const
{
   return _rect;
}

Lever::Lever(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(Lever).name());
}

Lever::~Lever()
{
   SaveState::getPlayerInfo()._inventory.removeUsedCallback(_handle_callback);
}

void Lever::preload()
{
   Audio::getInstance().addSample("mechanism_switch_off.wav");
   Audio::getInstance().addSample("mechanism_switch_on.wav");
   Audio::getInstance().addSample("mechanism_switch_lever_insert.wav");
}

void Lever::updateDirection()
{
   if (_target_state == State::Left)
   {
      _dir = -1;
   }
   else if (_target_state == State::Right)
   {
      _dir = 1;
   }
   else if (_target_state == State::Middle)
   {
      _dir = (_state_previous == State::Left) ? 1 : -1;
   };
}

void Lever::updateTargetPositionReached()
{
   if (_target_state == State::Left)
   {
      _reached = (_offset == 0);
   }
   else if (_target_state == State::Right)
   {
      _reached = (_offset == sprites_per_row - 1);
   }
   else if (_target_state == State::Middle)
   {
      _reached = (_offset == row_center);
   };
}

void Lever::setHandleAvailable(bool handle_available)
{
   _handle_available = handle_available;
}

const std::vector<std::string>& Lever::getTargetIds() const
{
   return _target_ids;
}

void Lever::update(const sf::Time& dt)
{
   const auto& player_rect = Player::getCurrent()->getPixelRectFloat();
   _player_at_lever = _rect.intersects(player_rect);

   if (!_handle_available)
   {
      // clang-format off
      constexpr auto no_handle_col = 10;
      constexpr auto no_handle_row = 2;
      const auto rect = sf::IntRect{
         PIXELS_PER_TILE * 3 * no_handle_col,
         PIXELS_PER_TILE * 3 * no_handle_row,
         PIXELS_PER_TILE * 3,
         PIXELS_PER_TILE * 3
      };
      // clang-format on

      _sprite.setTextureRect(rect);
      return;
   }

   updateTargetPositionReached();
   updateDirection();

   if (!_reached)
   {
      _offset += _dir;
   }
   else
   {
      if (_reached_previous)
      {
         _idle_time_s += dt.asSeconds();
      }
      else
      {
         _idle_time_s = 0.0f;
      }
   }

   updateSprite();

   _reached_previous = _reached;

   if (Player::getCurrent()->getControls()->isButtonBPressed() && _player_at_lever && _handle_available)
   {
      // block spamming
      using namespace std::chrono_literals;
      const auto now = std::chrono::high_resolution_clock::now();
      if (_last_toggle_time.has_value() && (now - _last_toggle_time.value()) < 1s)
      {
         return;
      }

      toggle();

      _last_toggle_time = now;
   }
}

void Lever::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   color.draw(_sprite);
}

std::optional<sf::FloatRect> Lever::getBoundingBoxPx()
{
   return sf::FloatRect(_rect.left, _rect.top, _rect.width, _rect.height);
}

void Lever::setEnabled(bool enabled)
{
   if (enabled)
   {
      _target_state = State::Right;
      _state_previous = State::Left;
   }
   else
   {
      _target_state = State::Left;
      _state_previous = State::Right;
   }

   // during setup the callbacks are not connected yet, so it's safe to call setEnabled during setup
   updateReceivers();
}

bool Lever::isEnabled() const
{
   return (_target_state == State::Right);
}

void Lever::updateReceivers()
{
   for (auto& cb : _callbacks)
   {
      cb(static_cast<int32_t>(_target_state));
   }
}

void Lever::toggle()
{
   if (_type == Type::TwoState)
   {
      switch (_target_state)
      {
         case State::Left:
            _target_state = State::Right;
            break;
         case State::Right:
            _target_state = State::Left;
            break;
         case State::Middle:
            break;
      }
   }

   else if (_type == Type::TriState)
   {
      switch (_target_state)
      {
         case State::Left:
         {
            _target_state = State::Middle;
            break;
         }
         case State::Middle:
         {
            if (_state_previous == State::Left)
            {
               _target_state = State::Right;
            }
            else
            {
               _target_state = State::Left;
            }
            break;
         }
         case State::Right:
         {
            _target_state = State::Middle;
            break;
         }
      }

      _state_previous = _target_state;
   }

   Audio::getInstance().playSample(
      _target_state == State::Left ? Audio::PlayInfo{"mechanism_switch_off.wav"} : Audio::PlayInfo{"mechanism_switch_on.wav"}
   );

   updateReceivers();
}

void Lever::addCallback(const Callback& callback)
{
   _callbacks.push_back(callback);
}

void Lever::setCallbacks(const std::vector<Callback>& callbacks)
{
   _callbacks = callbacks;
}

void Lever::serializeState(nlohmann::json& j)
{
   if (!_serialized)
   {
      return;
   }

   if (_object_id.empty())
   {
      Log::Warning() << "a lever has been configured to be serialized but it doesn't have any id";
      return;
   }

   j[_object_id] = {{"state", static_cast<int32_t>(_target_state)}};
}

void Lever::deserializeState(const nlohmann::json& j)
{
   _target_state = static_cast<State>(j.at("state").get<int32_t>());
   _enabled = (_target_state == State::Right);
   updateReceivers();
}
