
#ifndef FIREFLIES_H
#define FIREFLIES_H

#include "game/gamedeserializedata.h"
#include "game/gamemechanism.h"
#include "game/gamenode.h"

class Fireflies : public GameMechanism, public GameNode
{
public:
   struct Firefly
   {
      void update(const sf::Time& dt);
      void updateTextureRect();

      sf::Vector3f _position_3d;
      sf::Vector2f _position;
      sf::Sprite _sprite;
      sf::Time _elapsed;
      sf::FloatRect _rect_px;
      int32_t _current_frame{0};
      int32_t _instance_number{0};
      float _angle_x{0.0f};
      float _angle_y{0.0f};
      float _speed{1.0f};
      float _dir{1.0f};
      float _scale_vertical{1.0f};
      float _scale_horizontal{1.0f};
      float _animation_speed{0.0};
   };

   Fireflies(GameNode* parent = nullptr);
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;
   void deserialize(const GameDeserializeData& data);

private:
   sf::FloatRect _rect_px;
   std::vector<Firefly> _fireflies;
   std::shared_ptr<sf::Texture> _texture;
   int32_t _instance_counter = 0;
};

#endif // FIREFLIES_H
