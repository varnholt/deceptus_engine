#pragma once

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <SFML/Graphics.hpp>
#include <memory>

struct TmxObject;

/// \brief simulates and renders ambient dust guided by a flow-field texture.
class Dust : public GameMechanism, public GameNode
{
   /// \brief stores runtime data for one simulated dust particle.
   struct Particle
   {
      /// \brief respawns the particle at a random position inside the clip rectangle.
      /// \param rect spawn area in pixels.
      void spawn(sf::FloatRect& rect);
      sf::Vector3f _position;
      sf::Vector3f _direction;
      float _age = 0.0f;
      float _lifetime = 0.0f;
      float _z = 0.0f;
      float _center_reset_radius_sq = 0.0f;
   };

public:
   /// \brief creates a dust mechanism with no particles configured yet.
   /// \param parent parent node in the scene graph.
   Dust(GameNode* parent = nullptr);

   /// \brief unregisters the optional flow-field texture change listener.
   virtual ~Dust() override;

   /// \brief returns the mechanism registry name.
   /// \return string view containing `Dust`.
   std::string_view objectName() const override;

   /// \brief advances particle motion using flow-field direction and configured wind.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief draws all particles as batched triangle quads with age-based alpha.
   /// \param target render target.
   /// \param normal normal-map render target (unused).
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;

   /// \brief returns the clip rectangle used for simulation and visibility queries.
   /// \return clip rectangle in pixels.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief builds a dust instance from TMX properties, including particle and flow-field settings.
   /// \param parent parent node in the scene graph.
   /// \param data deserialize context with TMX object data and world access.
   /// \return configured dust mechanism.
   static std::shared_ptr<Dust> deserialize(GameNode* parent, const GameDeserializeData& data);

private:
   void rebuildFlowFieldCache();

   std::vector<Particle> _particles;
   sf::FloatRect _clip_rect;
   std::shared_ptr<sf::Texture> _flow_field_texture;
   sf::Image _flow_field_image;
   std::vector<sf::Vector3f> _flow_field_cache;  //!< pre-baked direction vectors indexed by pixel_y * image_width + pixel_x
   float _flow_field_scale_factor_x{0.0f};       //!< scale from clip-rect space to flow-field image space, x axis
   float _flow_field_scale_factor_y{0.0f};       //!< scale from clip-rect space to flow-field image space, y axis
   uint32_t _flow_field_image_width{0};          //!< flow-field image width for flat cache index computation
   sf::Vector3f _wind_direction;
   sf::Color _particle_color = {255, 255, 255, 255};
   float _particle_velocity = 100.0f;
   uint8_t _particle_size_px = 2;
   sf::VertexArray _vertices;
   bool _respawn_when_center_reached{false};
   std::optional<int32_t> _flowfield_listener_id;
};
