
#ifndef FIREFLIES_H
#define FIREFLIES_H

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

/// \brief spawns animated fireflies that orbit inside a configured rectangle.
class Fireflies : public GameMechanism, public GameNode
{
public:
   /// \brief stores per-firefly animation and movement parameters.
   struct Firefly
   {
      /// \brief creates one firefly sprite using the shared firefly texture.
      /// \param texture source texture containing firefly animation frames.
      Firefly(const sf::Texture& texture);

      /// \brief updates motion on a rotated lemniscate path and advances animation frames.
      /// \param dt elapsed frame time.
      void update(const sf::Time& dt);

      /// \brief updates the sprite texture rectangle for the current animation frame.
      void updateTextureRect();

      sf::Vector3f _position_3d;
      sf::Vector2f _position;
      std::unique_ptr<sf::Sprite> _sprite;
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

   /// \brief creates a fireflies mechanism.
   /// \param parent parent node in the scene graph.
   Fireflies(GameNode* parent = nullptr);

   /// \brief returns the mechanism registry name.
   /// \return string view containing `Fireflies`.
   std::string_view objectName() const override;

   /// \brief draws all firefly sprites.
   /// \param target render target.
   /// \param normal normal-map render target (unused).
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;

   /// \brief updates all firefly instances.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief returns bounds for mechanism queries.
   /// \return `std::nullopt` because this mechanism does not expose collision bounds.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief initializes area, count, animation speed, and randomized movement parameters from TMX data.
   /// \param data deserialize context with TMX object and properties.
   void deserialize(const GameDeserializeData& data);

private:
   sf::FloatRect _rect_px;
   std::vector<Firefly> _fireflies;
   std::shared_ptr<sf::Texture> _texture;
   int32_t _instance_counter = 0;
};

#endif  // FIREFLIES_H
