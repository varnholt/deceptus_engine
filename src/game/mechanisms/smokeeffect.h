#pragma once

#include <SFML/Graphics.hpp>

#include <memory>
#include <vector>

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

struct TmxObject;
struct TmxObjectGroup;

class SmokeEffect : public GameMechanism, public GameNode
{
public:
   enum class Mode
   {
      Smoke,
      Fog,
   };

   SmokeEffect(GameNode* parent = nullptr);
   std::string_view objectName() const override;

   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
   void update(const sf::Time& time) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   static std::shared_ptr<SmokeEffect> deserialize(GameNode* parent, const GameDeserializeData& data);

private:
   std::shared_ptr<sf::Texture> _texture;

   struct SmokeParticle
   {
      SmokeParticle(const sf::Texture& texture)
      {
         _sprite = std::make_unique<sf::Sprite>(texture);
      }

      std::unique_ptr<sf::Sprite> _sprite;
      float _rot = 0.0f;
      float _rot_dir = 1.0f;
      float _time_offset = 0.0f;

      sf::Vector2f _offset;
      sf::Vector2f _center;
   };

   std::vector<SmokeParticle> _particles;
   sf::Time _elapsed;

   float _pixel_ratio = 1.0f;
   float _velocity = 1.0f;
   sf::Vector2u _size_px;
   sf::Vector2f _offset_px;
   sf::FloatRect _bounding_box_px;
   sf::BlendMode _blend_mode = sf::BlendAdd;
   sf::Color _layer_color = {255, 255, 255, 255};
   sf::Color _particle_color = {255, 255, 255, 25};
   Mode _mode = Mode::Smoke;
   std::unique_ptr<sf::RenderTexture> _render_texture;
   sf::VertexArray _batched_vertices;
};
