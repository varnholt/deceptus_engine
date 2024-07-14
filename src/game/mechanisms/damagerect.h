#ifndef DAMAGERECT_H
#define DAMAGERECT_H

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

class DamageRect : public GameMechanism, public GameNode
{
public:
   DamageRect(GameNode* parent = nullptr);
   void update(const sf::Time& dt) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;
   void setup(const GameDeserializeData& data);

private:
   sf::FloatRect _rect;
   int32_t _damage{100};
};

#endif // DAMAGERECT_H
