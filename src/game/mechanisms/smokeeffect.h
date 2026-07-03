#pragma once

#include <SFML/Graphics.hpp>

#include <memory>
#include <vector>

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

struct TmxObject;
struct TmxObjectGroup;

/// \brief renders animated smoke or fog particles inside a rectangular area.
class SmokeEffect : public GameMechanism, public GameNode
{
public:
   /// \brief particle animation style used by this effect.
   enum class Mode
   {
      Smoke,
      Fog,
   };

   /// \brief creates a smoke effect with default texture and render settings.
   /// \param parent owning game node in the scene graph.
   SmokeEffect(GameNode* parent = nullptr);

   /// \brief returns the mechanism type name used by the serialization system.
   /// \return constant string view containing "SmokeEffect".
   std::string_view objectName() const override;

   /// \brief renders all particles into an offscreen texture and draws the layer.
   /// \param color color render target.
   /// \param normal normal-map render target, unused by this mechanism.
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;

   /// \brief renders all particles and draws the layer with explicit render states (used in WASM to carry the level view).
   /// \param color color render target.
   /// \param normal normal-map render target, unused by this mechanism.
   /// \param states render states to apply to the final layer draw.
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal, const sf::RenderStates& states) override;
   using GameMechanism::draw;

   /// \brief animates particle rotation, offsets, color, and batched vertices.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief returns the effect rectangle in pixel space.
   /// \return bounding box used for chunk activation and culling.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief creates and configures a smoke effect from tmx object properties.
   /// \param parent owning game node in the scene graph.
   /// \param data deserialization data with particle counts, colors, and mode.
   /// \return configured smoke effect instance.
   static std::shared_ptr<SmokeEffect> deserialize(GameNode* parent, const GameDeserializeData& data);

private:
   std::shared_ptr<sf::Texture> _texture;

   /// \brief stores one particle sprite and its orbit animation parameters.
   struct SmokeParticle
   {
      SmokeParticle(const sf::Texture& texture)
      {
         _sprite = std::make_unique<sf::Sprite>();
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
