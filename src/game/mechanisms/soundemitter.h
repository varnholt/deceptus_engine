#ifndef SOUNDEMITTER_H
#define SOUNDEMITTER_H

#include "gamedeserializedata.h"
#include "gamemechanism.h"
#include "gamenode.h"

class SoundEmitter : public GameMechanism, public GameNode
{
public:
   SoundEmitter(GameNode* parent);
   ~SoundEmitter() override;

   void setAudioEnabled(bool enabled) override;
   void setReferenceVolume(float volume) override;

   static std::shared_ptr<SoundEmitter> deserialize(GameNode* parent, const GameDeserializeData& data);

   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   sf::Vector2f _position;
   sf::FloatRect _rect;
   sf::Vector2f _size;

   bool _looped{true};
   std::string _filename;
   std::optional<int32_t> _thread_id;
private:
   void stopPlaying();
};

#endif // SOUNDEMITTER_H
