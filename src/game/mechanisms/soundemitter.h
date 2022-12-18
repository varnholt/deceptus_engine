#ifndef SOUNDEMITTER_H
#define SOUNDEMITTER_H

#include "gamedeserializedata.h"
#include "gamemechanism.h"
#include "gamenode.h"

class SoundEmitter : public GameMechanism, public GameNode
{
public:
   SoundEmitter(GameNode* parent);

   static std::shared_ptr<SoundEmitter> deserialize(GameNode* parent, const GameDeserializeData& data);

   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   sf::Vector2f _position;
   sf::FloatRect _rect;
   sf::Vector2f _size;
   float _volume{0.0f};

   bool _looped{true};
   std::string _filename;
};

#endif // SOUNDEMITTER_H
