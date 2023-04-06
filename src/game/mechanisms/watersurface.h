#ifndef WATERSURFACE_H
#define WATERSURFACE_H

#include "gamedeserializedata.h"
#include "gamemechanism.h"
#include "gamenode.h"

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
   };

   WaterSurface(GameNode* parent, const GameDeserializeData& data);

   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   void splash(int32_t index, float velocity);

private:
   void updateVertices(int32_t start_index = 0);
   sf::FloatRect _bounding_box;
   std::vector<Segment> _segments;
   sf::Shader _shader;
   std::optional<bool> _player_was_in_water;
   sf::VertexArray _vertices;
   float _segment_width{0.0f};
   sf::Texture _gradient;

   sf::RenderTexture _render_texture;
   sf::Sprite render_texture_sprite;
   std::optional<float> _pixel_ratio;
};

#endif // WATERSURFACE_H
