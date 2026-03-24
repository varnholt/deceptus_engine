#ifndef SOUNDEMITTER_H
#define SOUNDEMITTER_H

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

/// \brief plays positional ambient audio configured from a map object.
class SoundEmitter : public GameMechanism, public GameNode
{
public:
   /// \brief creates a sound emitter mechanism.
   /// \param parent owning game node in the scene graph.
   SoundEmitter(GameNode* parent);

   /// \brief stops any active sample thread on destruction.
   ~SoundEmitter() override;

   /// \brief returns the mechanism type name used by the serialization system.
   /// \return constant string view containing "SoundEmitter".
   std::string_view objectName() const override;

   /// \brief starts or stops the configured sample when audio toggles.
   /// \param enabled true to allow playback, false to stop it.
   void setAudioEnabled(bool enabled) override;

   /// \brief updates reference volume and applies it to the active sample.
   /// \param volume target reference volume.
   void setReferenceVolume(float volume) override;

   /// \brief creates an emitter from tmx properties and preloads its sample.
   /// \param parent owning game node in the scene graph.
   /// \param data deserialization data with bounds, ranges, and filename.
   /// \return configured sound emitter instance.
   static std::shared_ptr<SoundEmitter> deserialize(GameNode* parent, const GameDeserializeData& data);

   /// \brief returns the emitter bounds in pixel space.
   /// \return rectangle used for chunk registration and editor visualization.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   sf::Vector2f _position;
   sf::FloatRect _rect;
   sf::Vector2f _size;

   bool _looped{true};
   std::string _filename;
   std::optional<int32_t> _thread_id;

private:
   /// \brief stops the currently playing sample when one is active.
   void stopPlaying();
};

#endif  // SOUNDEMITTER_H
