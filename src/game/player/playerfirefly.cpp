#include "playerfirefly.h"

#include "game/constants.h"
#include "game/io/texturepool.h"
#include "game/player/playerregistry.h"

#include <cmath>

namespace
{
constexpr auto frame_count = 4;
constexpr auto orbit_radius_x_px = 96.0f;
constexpr auto orbit_radius_y_px = 48.0f;
constexpr auto catch_up_speed = 5.0f;
constexpr auto z_switch_threshold = 0.6f;
constexpr auto z_in_front = static_cast<int32_t>(ZDepth::Player) + 1;
constexpr auto z_behind = static_cast<int32_t>(ZDepth::Player) - 1;
}  // namespace

PlayerFirefly::PlayerFirefly(GameNode* parent) : GameNode(parent)
{
   _texture = TexturePool::getInstance().get("data/sprites/firefly.png");
   _sprite = std::make_unique<sf::Sprite>(*_texture);
   _sprite->setTextureRect({{0, 0}, {PIXELS_PER_TILE, PIXELS_PER_TILE}});
   _z_index = z_in_front;
   _enabled = false;
   _visible = false;
}

std::string_view PlayerFirefly::objectName() const
{
   return "PlayerFirefly";
}

void PlayerFirefly::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   if (!_visible)
   {
      return;
   }
   target.draw(*_sprite);
}

void PlayerFirefly::update(const sf::Time& dt)
{
   if (!_enabled)
   {
      return;
   }

   const auto player = PlayerRegistry::getFirst();
   if (!player)
   {
      return;
   }

   const auto player_position_px = player->getPixelPositionFloat();

   if (!_initialized)
   {
      _virtual_center_px = player_position_px;
      _initialized = true;
   }

   _elapsed += dt;
   const auto time_s = _elapsed.asSeconds();
   const auto delta_s = dt.asSeconds();

   _virtual_center_px += (player_position_px - _virtual_center_px) * catch_up_speed * delta_s;

   const auto raw_x = std::cos(time_s * _speed * _dir);
   const auto raw_y = std::sin(2.0f * time_s * _speed * _dir) * 0.5f;

   _position_px.x = _virtual_center_px.x + raw_x * orbit_radius_x_px;
   _position_px.y = _virtual_center_px.y + raw_y * orbit_radius_y_px;

   _sprite->setPosition(_position_px);
   _sprite->setOrigin({static_cast<float>(PIXELS_PER_TILE) * 0.5f, static_cast<float>(PIXELS_PER_TILE) * 0.5f});

   if (std::abs(raw_x) > z_switch_threshold)
   {
      _z_index = (raw_x > 0.0f) ? z_in_front : z_behind;
   }

   updateTextureRect();
}

std::optional<sf::FloatRect> PlayerFirefly::getBoundingBoxPx()
{
   return std::nullopt;
}

void PlayerFirefly::setEnabled(bool enabled)
{
   if (!enabled)
   {
      _initialized = false;
   }
   GameMechanism::setEnabled(enabled);
   _visible = enabled;
}

void PlayerFirefly::updateTextureRect()
{
   const auto elapsed_s = _elapsed.asSeconds();
   const auto new_frame = static_cast<int32_t>(elapsed_s * _animation_speed) % frame_count;

   if (new_frame != _current_frame)
   {
      _current_frame = new_frame;
      _sprite->setTextureRect({{_current_frame * PIXELS_PER_TILE, 0}, {PIXELS_PER_TILE, PIXELS_PER_TILE}});
   }
}
