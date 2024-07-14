#ifndef WATERSURFACE_H
#define WATERSURFACE_H

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

class WaterSurface : public GameMechanism, public GameNode
{
public:
   struct Segment
   {
      Segment() = default;

      void update(float dampening, float tension);
      void resetDeltas();

      float _height{0.0f};
      float _target_height{0.0f};
      float _velocity{0.0f};

      float _delta_left{0.0f};
      float _delta_right{0.0f};

      float _clamp_scale{1.0f};
   };

   struct Config
   {
      float _tension = 0.025f;
      float _dampening = 0.025f;
      float _spread = 0.25f;

      float _animation_speed = 10.0f;
      float _splash_factor = 50.0f;
   };

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

   WaterSurface(GameNode* parent, const GameDeserializeData& data);

   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   void splash(int32_t index, float velocity);
   static void addEmitter(GameNode* parent, const GameDeserializeData& data);
   static void merge();

private:
   void updateVertices(int32_t start_index = 0);
   void updateEmitters(float elapsed_s);
   sf::FloatRect _bounding_box;
   std::vector<Segment> _segments;
   std::optional<bool> _player_was_in_water;
   sf::VertexArray _vertices;
   float _segment_width{0.0f};
   std::shared_ptr<sf::Texture> _gradient;
   uint8_t _opacity{200};

   sf::RenderTexture _render_texture;
   sf::Sprite render_texture_sprite;
   std::optional<float> _pixel_ratio;
   Config _config;
   std::vector<SplashEmitter> _emitters;
};

#endif // WATERSURFACE_H
