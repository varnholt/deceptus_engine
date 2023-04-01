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

   WaterSurface() = default;

   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   virtual void setup(const GameDeserializeData& data);

private:
   sf::FloatRect _bounding_box;
   std::vector<Segment> _segments;
};

#endif // WATERSURFACE_H
