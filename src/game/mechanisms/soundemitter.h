#ifndef SOUNDEMITTER_H
#define SOUNDEMITTER_H

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

/// \brief plays positional ambient audio configured from a map object.
/// \note deliberately does not call addChunks: distance attenuation and the fade are precisely what have to keep
///       working while the player is far away, which is the opposite of what chunk culling provides.
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

   /// \brief advances the fade and starts or stops the sample once it has run its course.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief stops the sample when the player leaves audio range.
   /// \param enabled true to allow playback, false to stop it.
   void setAudioEnabled(bool enabled) override;

   /// \brief applies the distance-scaled volume to the sample that is currently playing.
   /// \param volume volume computed by the volume updater.
   void setVolume(float volume) override;

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

   /// \brief applies the current volume and fade state to the sample that is playing.
   void applyVolume();

   /// \brief returns the volume the sample should be heard at right now.
   /// \return distance-scaled volume weighted by the fade.
   float computeVolume() const;

   float _fade_duration_s{0.0f};  //!< time the sample takes to fade in or out when the mechanism is enabled or disabled
   float _fade_factor{1.0f};      //!< current position of the fade, 0 is silent and 1 is the full distance-scaled volume
};

#endif  // SOUNDEMITTER_H
