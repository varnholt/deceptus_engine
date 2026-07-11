#ifndef WATERSURFACE_H
#define WATERSURFACE_H

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

/// \brief simulates and renders a deformable water surface with splash propagation.
class WaterSurface : public GameMechanism, public GameNode
{
public:
   /// \brief one simulated spring segment along the water surface.
   struct Segment
   {
      Segment() = default;

       /// \brief integrates this segment toward its target height.
       /// \param dampening velocity dampening factor.
       /// \param tension spring tension factor.
       void update(float dampening, float tension);

       /// \brief clears per-step neighbor transfer deltas.
       void resetDeltas();

      float _height{0.0f};
      float _target_height{0.0f};
      float _velocity{0.0f};

      float _delta_left{0.0f};
      float _delta_right{0.0f};

      float _clamp_scale{1.0f};
   };

   /// \brief wave simulation tuning parameters.
   struct Config
   {
      float _tension = 0.025f;
      float _dampening = 0.025f;
      float _spread = 0.25f;

      float _animation_speed = 10.0f;
      float _splash_factor = 50.0f;
   };

   /// \brief periodic splash source configuration attached to a surface.
   struct SplashEmitter
   {
      float _x_from_px{0.0f};
      float _x_to_px{0.0f};
      float _interval_min_s{0.0f};
      float _interval_max_s{0.0f};
      float _velocity{0.0f};
      float _width_px{0.0f};
      int32_t _count{1};

      float _elapsed_s{0.0f};
      float _interval_s{0.0f};

      std::string _parent_reference;
      sf::FloatRect _bounding_box;
      float _x_offset_to_parent_px{0.0f};
   };

   /// \brief builds wave segments, mesh vertices, and optional low-resolution render target.
   /// \param parent owning game node in the scene graph.
   /// \param data deserialize context with surface bounds and property values.
   WaterSurface(GameNode* parent, const GameDeserializeData& data);

   /// \brief returns the mechanism type name used by the serialization system.
   /// \return constant string view containing "WaterSurface".
   std::string_view objectName() const override;

   /// \brief draws the animated water gradient strip.
   /// \param color color render target.
   /// \param normal normal-map render target, unused by this mechanism.
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;

   /// \brief draws the animated water gradient strip with explicit render states (used in WASM to carry the level view).
   /// \param color color render target.
   /// \param normal normal-map render target, unused by this mechanism.
   /// \param states render states to apply.
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal, const sf::RenderStates& states) override;
   using GameMechanism::draw;

   /// \brief updates wave simulation, player splashes, and emitter-generated disturbances.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief returns the surface area rectangle in pixel space.
   /// \return bounding box used for interaction and chunk registration.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief injects vertical velocity into a wave segment to create a splash.
   /// \param index target segment index near the splash location.
   /// \param velocity velocity impulse applied to the segment.
   void splash(int32_t index, float velocity);

   /// \brief stores a splash emitter definition to be attached during merge.
   /// \param parent owning game node in the scene graph, unused for emitters.
   /// \param data deserialization data for the emitter object.
   static void addEmitter(GameNode* parent, const GameDeserializeData& data);

   /// \brief attaches queued emitters to their referenced water surfaces.
   static void merge();

private:
   /// \brief updates alternating top or bottom vertices of the strip mesh.
   /// \param start_index first vertex parity to update, typically 0 for top or 1 for bottom.
   void updateVertices(int32_t start_index = 0);

   /// \brief advances emitter timers and emits splash impulses when timers elapse.
   /// \param elapsed_s elapsed seconds since the previous frame.
   void updateEmitters(float elapsed_s);
   sf::FloatRect _bounding_box;
   std::vector<Segment> _segments;
   std::optional<bool> _player_was_in_water;
   sf::VertexArray _vertices;
   float _segment_width{0.0f};
   std::shared_ptr<sf::Texture> _gradient;
   uint8_t _opacity{200};

   std::unique_ptr<sf::RenderTexture> _render_texture;
   std::unique_ptr<sf::Sprite> render_texture_sprite;
   std::optional<float> _pixel_ratio;
   Config _config;
   std::vector<SplashEmitter> _emitters;
};

#endif  // WATERSURFACE_H
