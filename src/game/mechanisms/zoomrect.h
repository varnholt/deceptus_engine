#ifndef ZOOMRECT_H
#define ZOOMRECT_H

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

class ZoomRect : public GameMechanism, public GameNode
{
public:
   enum class Normalization
   {
      None,
      Linear
   };

   struct ZoomFactor
   {
      float _radius{1.0f};
      float _factor{1.0f};
   };

   ZoomRect(GameNode* parent);
   void update(const sf::Time& dt) override;
   void setup(const GameDeserializeData& data);

private:
   Normalization _normalization{Normalization::Linear};
   sf::FloatRect _rect_px;
   sf::Vector2f _center_px;
   float _radius_px{0.0};
   std::vector<ZoomFactor> _zoom_factors;
   bool _within_rect_in_previous_frame{false};
};

#endif  // ZOOMRECT_H
