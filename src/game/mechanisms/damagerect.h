#ifndef DAMAGERECT_H
#define DAMAGERECT_H

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

/// \brief invisible hazard volume that damages the player every frame while overlapping.
class DamageRect : public GameMechanism, public GameNode
{
public:
   /// \brief creates an empty damage volume.
   /// \param parent parent node in the scene graph.
   DamageRect(GameNode* parent = nullptr);
   /// \brief returns the mechanism type identifier.
   /// \return non-owning string view with value "DamageRect".
   std::string_view objectName() const override;

   /// \brief applies configured damage to the player when the player overlaps the rectangle.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;
   /// \brief returns hazard bounds in pixel coordinates.
   /// \return rectangle used for overlap checks.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;
   /// \brief initializes rectangle bounds and damage amount from tmx properties.
   /// \param data deserialize context with tmx object values.
   void setup(const GameDeserializeData& data);

private:
   sf::FloatRect _rect;
   int32_t _damage{100};
};

#endif  // DAMAGERECT_H
