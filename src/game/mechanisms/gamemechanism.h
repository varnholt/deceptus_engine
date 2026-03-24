#pragma once

#include "game/audio/audiorange.h"
#include "game/audio/audioupdatedata.h"
#include "game/constants.h"
#include "game/level/chunk.h"
#include "game/level/hitbox.h"

#include "SFML/Graphics.hpp"

#include "json/json.hpp"

#include <cstdint>
#include <optional>

struct Room;

/// \brief defines the shared interface and common state for all level mechanisms.
class GameMechanism
{
public:
   /// \brief constructs a mechanism with default enabled and visible state.
   GameMechanism() = default;
   /// \brief destroys the mechanism base object.
   virtual ~GameMechanism() = default;

   /// \brief returns the serialized type name used by mechanism systems.
   /// \return non-owning name of the concrete mechanism type.
   virtual std::string_view objectName() const = 0;

   /// \brief draws the mechanism to color and normal render targets.
   /// \param target color render target.
   /// \param normal normal-map render target.
   virtual void draw(sf::RenderTarget& target, sf::RenderTarget& normal);
   /// \brief updates mechanism logic for one frame.
   /// \param dt elapsed frame time.
   virtual void update(const sf::Time& dt);

   /// \brief checks whether gameplay logic for this mechanism is enabled.
   /// \return true when the mechanism is enabled.
   virtual bool isEnabled() const;
   /// \brief enables or disables the mechanism.
   /// \param enabled true to enable the mechanism, false to disable it.
   virtual void setEnabled(bool enabled);
   /// \brief toggles the enabled flag.
   virtual void toggle();

   /// \brief checks whether the mechanism should currently be rendered.
   /// \return true when the mechanism is visible.
   virtual bool isVisible() const;
   /// \brief sets render visibility for this mechanism.
   /// \param visible true to render the mechanism.
   virtual void setVisible(bool visible);

   /// \brief preloads assets such as audio samples before gameplay starts.
   virtual void preload();

   // audio related
   /// \brief checks whether this mechanism participates in distance-based audio updates.
   /// \return true when the mechanism exposes audio behavior.
   virtual bool hasAudio() const;
   /// \brief returns the audio attenuation range used by the audio updater.
   /// \return configured audio range when available.
   virtual std::optional<AudioRange> getAudioRange() const;
   /// \brief checks whether dynamic audio updates are currently enabled.
   /// \return true when runtime audio updates are enabled.
   virtual bool isAudioEnabled() const;
   /// \brief enables or disables runtime audio updates for the mechanism.
   /// \param audio_enabled true to enable runtime audio updates.
   virtual void setAudioEnabled(bool audio_enabled);
   /// \brief stores the reference volume used as baseline for distance scaling.
   /// \param volume unscaled reference volume.
   virtual void setReferenceVolume(float volume);  // this is read only for the volume updater
   /// \brief sets the current runtime volume value.
   /// \param volume current volume after audio update processing.
   virtual void setVolume(float volume);
   /// \brief returns the reference volume baseline.
   /// \return reference volume used by the audio updater.
   virtual float getReferenceVolume() const;
   /// \brief returns how the audio updater should evaluate this mechanism.
   /// \return configured audio update behavior.
   virtual AudioUpdateBehavior getAudioUpdateBehavior() const;
   /// \brief configures how the audio updater handles this mechanism.
   /// \param audio_update_behavior audio update strategy to store.
   virtual void setAudioUpdateBehavior(AudioUpdateBehavior audio_update_behavior);
   /// \brief returns room ids used to gate updates by active room.
   /// \return list of room ids associated with this mechanism.
   virtual const std::vector<int32_t>& getRoomIds() const;
   /// \brief replaces the room-id filter used by this mechanism.
   /// \param room_ids room ids where this mechanism is considered active.
   virtual void setRoomIds(const std::vector<int32_t>& room_ids);
   /// \brief appends one room id to the room filter.
   /// \param room_id room id to append.
   virtual void addRoomId(int32_t room_id);

   // use when mechanism update/draw calls are expensive
   /// \brief checks whether chunk culling data has been assigned.
   /// \return true when at least one chunk is stored.
   virtual bool hasChunks() const;
   /// \brief returns the chunk list used for coarse update and draw culling.
   /// \return stored chunk coordinates.
   virtual const std::vector<Chunk>& getChunks() const;
   /// \brief computes and stores all chunks touched by a pixel-space bounding box.
   /// \param bounding_box mechanism bounds in pixel coordinates.
   virtual void addChunks(const sf::FloatRect& bounding_box);

   /// \brief returns render z order for this mechanism.
   /// \return z index used by the renderer.
   virtual int32_t getZ() const;
   /// \brief sets render z order for this mechanism.
   /// \param z z index to assign.
   virtual void setZ(const int32_t& z);

   /// \brief returns mechanism bounds in pixels for culling and interactions.
   /// \return pixel bounding box when the mechanism has one.
   virtual std::optional<sf::FloatRect> getBoundingBoxPx() = 0;

   /// \brief serializes mechanism runtime state into save data.
   /// \param json json object to write state fields into.
   virtual void serializeState(nlohmann::json&);
   /// \brief restores mechanism runtime state from save data.
   /// \param json json object containing previously serialized state.
   virtual void deserializeState(const nlohmann::json&);
   /// \brief checks whether this mechanism contributes to save-state serialization.
   /// \return true when serializeState and deserializeState are expected to be used.
   virtual bool isSerialized() const;

   /// \brief checks whether this mechanism can receive damage and be destroyed.
   /// \return true when hit points and destruction are implemented.
   virtual bool isDestructible() const;
   /// \brief returns hitboxes that can receive weapon hits.
   /// \return hitbox list for destructible interaction checks.
   virtual const std::vector<Hitbox>& getHitboxes();
   /// \brief applies damage to the mechanism.
   /// \param damage amount of damage to apply.
   virtual void hit(int32_t damage);

protected:
   int32_t _z_index{0};
   bool _enabled{true};
   bool _visible{true};
   bool _serialized{false};
   bool _observed{false};

   // audio related
   bool _has_audio{false};
   bool _audio_enabled{false};
   float _reference_volume{0.0f};
   AudioUpdateData _audio_update_data;

   std::vector<Chunk> _chunks;
   MechanismVersion _version = MechanismVersion::Version1;
};
