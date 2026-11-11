#pragma once

#include "game/audio/audiorange.h"
#include "game/audio/audioupdatedata.h"
#include "game/constants.h"
#include "game/level/chunk.h"
#include "game/level/hitbox.h"
#include "game/mechanisms/gamemechanismobserver.h"

#include "SFML/Graphics.hpp"

#include "json/json.hpp"

#include <cstdint>
#include <optional>
#include <string>
#if defined(DEVELOPMENT_MODE) && !defined(DECEPTUS_VRSFML)
#include <memory>
#include "game/debug/mechanism_property_bindings.h"
#endif

struct Room;
#if defined(DEVELOPMENT_MODE) && !defined(DECEPTUS_VRSFML)
struct TmxObject;
#endif

/// \brief identifies which render stage a mechanism contributes to.
///
/// Ordinary mechanisms draw themselves into the level render targets. A post processing mechanism
/// cannot: its pass runs after the level has been composited, and for the frame-wide scope even
/// after the hud. Those mechanisms therefore do not draw at all, they hand a shader to the post
/// processing pass instead.
enum class MechanismRenderStage
{
   Level,          //!< drawn into the level render targets like an ordinary mechanism
   PostProcessing  //!< contributes a full screen shader pass applied after the level
};

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

   /// \brief draws the mechanism with explicit render states (used in WASM to carry the level view).
   /// \param target color render target.
   /// \param normal normal-map render target.
   /// \param states render states to apply (carries .view for WASM camera transform).
   virtual void draw(sf::RenderTarget& target, sf::RenderTarget& normal, const sf::RenderStates& states);

   /// \brief places this mechanism's sprites for the frame about to be drawn.
   ///
   /// Called once per frame, after the simulation steps for that frame have run. Mechanisms that move
   /// override it and position their sprites between the state they had before the last step and the
   /// state they have now, via RenderInterpolation - the simulation advances at a fixed rate, so a
   /// frame drawn between two steps would otherwise repeat a position it has already shown.
   ///
   /// Sprite positions belong here rather than in draw: draw draws.
   virtual void updateSpritePositions();

   /// \brief updates mechanism logic for one frame.
   /// \param dt elapsed frame time.
   virtual void update(const sf::Time& dt);

   /// \brief checks whether gameplay logic for this mechanism is enabled.
   /// \return true when the mechanism is enabled.
   virtual bool isEnabled() const;

   /// \brief enables or disables the mechanism.
   /// \param enabled true to enable the mechanism, false to disable it.
   virtual void setEnabled(bool enabled);

#if defined(DEVELOPMENT_MODE) && !defined(DECEPTUS_VRSFML)
   /// \brief returns bounds used by development-editor picking and highlights.
   /// \return current visual bounds when specialized, otherwise gameplay bounds; null when unavailable.
   /// \note querying editor bounds does not modify simulation or gameplay interaction geometry.
   [[nodiscard]] virtual std::optional<sf::FloatRect> get_editor_bounds();
   /// \brief validates and applies an explicit development-editor property change.
   /// \param name authored property name.
   /// \param value candidate text accepted by the shared typed binding.
   /// \return true when the available binding accepts and applies the value.
   bool setInspectorProperty(const std::string& name, const std::string& value);
   /// \brief reads a development property, initializing adapters lazily on first access.
   /// \param name authored property name.
   /// \return encoded runtime value, or null when no available live binding exists.
   [[nodiscard]] std::optional<std::string> getInspectorProperty(const std::string& name);
   /// \brief checks development-only live support, initializing adapters lazily when needed.
   /// \param name authored property name.
   /// \return true when the property has an available runtime binding.
   [[nodiscard]] bool supportsInspectorProperty(const std::string& name);
   /// \brief checks a development property edit without changing runtime state.
   /// \param name authored property name.
   /// \param value candidate encoded runtime value.
   /// \return true when an available binding accepts the candidate.
   [[nodiscard]] bool validateInspectorProperty(const std::string& name, const std::string& value);
   /// \brief returns the exact authoring snapshot retained at the mechanism factory boundary.
   /// \return retained source snapshot, possibly null for runtime-generated mechanisms.
   [[nodiscard]] const std::shared_ptr<TmxObject>& getInspectorSource() const
   {
      return _inspector_source;
   }
   /// \brief associates a mechanism with the authoring source that created it.
   /// \param source shared source snapshot, or null when no source exists.
   /// \param layer true when the source id belongs to a tile layer rather than an object.
   void setInspectorSource(const std::shared_ptr<TmxObject>& source, bool layer = false)
   {
      _inspector_source = source;
      _inspector_layer = layer;
   }
   /// \brief identifies which TMX namespace contains the retained source id.
   /// \return true for layer sources, false for object sources.
   [[nodiscard]] bool isInspectorLayer() const
   {
      return _inspector_layer;
   }
#endif

   /// \brief toggles the enabled flag.
   virtual void toggle();

   /// \brief checks whether the mechanism should currently be rendered.
   /// \return true when the mechanism is visible.
   virtual bool isVisible() const;

   /// \brief checks whether this mechanism should be drawn after the lighting pass.
   /// when true the mechanism is excluded from the lit scene and composited onto the
   /// deferred target afterwards so normal-map lighting does not render on top of it.
   /// \return true when the mechanism opts out of the lighting pass.
   virtual bool isPostLighting() const;

   /// \brief returns the render stage this mechanism contributes to.
   /// \return stage deciding whether the mechanism draws itself or feeds the post processing pass.
   virtual MechanismRenderStage getRenderStage() const;

   /// \brief checks whether this mechanism is a screen overlay drawn on top of all other layers,
   /// including post-lighting layers, so it is always visible regardless of lighting compositing.
   /// \return true when the mechanism should be drawn in the overlay pass.
   virtual bool isOverlay() const;

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

   /// \brief reads one named runtime property so level scripts can derive their state from this mechanism
   /// instead of keeping a duplicate copy of it in the save state.
   /// \param property_name name of the property to read.
   /// \return property value when this mechanism exposes the property, std::nullopt otherwise.
   virtual std::optional<GameMechanismObserver::LuaVariant> getProperty(const std::string& property_name) const;

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
#if defined(DEVELOPMENT_MODE) && !defined(DECEPTUS_VRSFML)
   /// \brief declares additional live adapters once, after construction, on first editor access.
   /// \param properties shared registry receiving mechanism-specific bindings; common adapters are already present.
   /// \note extend a specialized parent's hook when its additional adapters are required.
   virtual void configureInspectorProperties(MechanismPropertyBindings& properties)
   {
   }
   /// \brief registers common state through the same typed API as mechanism-specific fields.
   /// \param properties registry receiving enabled, visible, z, observation, and serialization adapters.
   void configureCommonInspectorProperties(MechanismPropertyBindings& properties);
   /// \brief creates the registry lazily and incorporates available shared light-component adapters.
   /// \return owned registry for this mechanism's current lifetime.
   MechanismPropertyBindings& inspectorProperties();
   std::unique_ptr<MechanismPropertyBindings> _inspector_properties;  //!< lazy editor-only adapters; absent from the gameplay update path.
   std::shared_ptr<TmxObject> _inspector_source;                      //!< retained source snapshot for precise map writes.
   bool _inspector_layer{false};                                      //!< source-id namespace discriminator.
#endif
   int32_t _z_index{0};
   bool _enabled{true};
   bool _visible{true};
   bool _serialized{false};
   bool _observed{false};
   bool _post_lighting{false};  //!< when true, drawn after the lighting pass so normal-map lighting does not composite on top
   bool _is_overlay{false};     //!< when true, drawn after all other layers including post-lighting layers

   MechanismRenderStage _render_stage{MechanismRenderStage::Level};  //!< render stage this mechanism contributes to

   // audio related
   bool _has_audio{false};
   bool _audio_enabled{false};
   float _reference_volume{0.0f};
   AudioUpdateData _audio_update_data;

   std::vector<Chunk> _chunks;
   MechanismVersion _version = MechanismVersion::Version1;
};
