#pragma once

#include "game/audio/soundrotation.h"
#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <SFML/Graphics.hpp>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

/// \brief applies directional force to the player while inside a rectangular wind area, blows animated
///        leaves through that area and plays one or more wind samples while the player is nearby.
///
/// The direction is stored as a normalized vector; how hard the wind pushes is configured separately
/// through `strength`. That way a level can be re-tuned without having to rebalance the direction, and
/// the same vector can drive the force and the leaf movement.
///
/// How loud the currently playing sample is can additionally modulate the force and the leaf velocity,
/// through `sound_strength_influence` and `leaf_sound_influence` respectively. The two are independent,
/// so a zone that does not push the player at all can still have its leaves surge with the gusts.
class Wind : public GameMechanism, public GameNode
{
public:
   /// \brief configuration of the leaves blown through the wind area.
   struct LeafSettings
   {
      std::string _texture_path{"data/sprites/leaves_fall.png"};  //!< sprite sheet with the animation frames laid out horizontally
      int32_t _count{0};                 //!< number of leaves alive at the same time; 0 disables the leaves entirely
      int32_t _frame_size_px{16};        //!< width and height of one animation frame
      float _velocity_px_s{60.0f};       //!< travel speed along the wind direction at full strength
      float _jitter_amount{0.35f};       //!< sideways drift relative to the travel speed
      float _jitter_frequency_hz{0.8f};  //!< how often a leaf wanders from one side to the other per second
      float _animation_speed{8.0f};      //!< animation frames per second
      float _scale_min{1.0f};            //!< lower bound of the randomized per-leaf scale
      float _scale_max{1.0f};            //!< upper bound of the randomized per-leaf scale
      float _alpha{1.0f};                //!< opacity of the leaf sprites
      float _sound_influence{0.0f};      //!< 0 keeps the velocity constant, 1 makes it follow the sample loudness
   };

   /// \brief runtime state of one leaf travelling through the wind area.
   struct Leaf
   {
      std::unique_ptr<sf::Sprite> _sprite;
      sf::Vector2f _position_px;
      float _age_s{0.0f};
      float _lifetime_s{0.0f};
      float _jitter_phase{0.0f};         //!< keeps the leaves from wandering in sync
      float _jitter_frequency_hz{1.0f};  //!< randomized around the configured frequency
      float _speed_factor{1.0f};         //!< randomized around the configured velocity
      float _animation_offset_s{0.0f};   //!< keeps the leaves from animating in sync
      float _scale{1.0f};
      int32_t _frame{-1};
   };

   /// \brief creates a wind mechanism instance.
   /// \param parent owning game node in the scene graph.
   explicit Wind(GameNode* parent = nullptr);
   ~Wind() override;

   /// \brief returns the mechanism type name used by the serialization system.
   /// \return constant string view containing "Wind".
   std::string_view objectName() const override;

   /// \brief applies configured force to the player body when inside the wind area, moves the leaves
   ///        and advances the sample playback.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief draws all leaves travelling through the wind area.
   /// \param target render target.
   /// \param normal normal-map render target.
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;

   /// \brief draws all leaves with explicit render states (used in WASM to carry the level view).
   /// \param target render target.
   /// \param normal normal-map render target.
   /// \param states render states to apply.
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal, const sf::RenderStates& states) override;
   using GameMechanism::draw;

   /// \brief starts or stops the wind samples as the player enters or leaves audio range.
   /// \param audio_enabled true while the mechanism is within audio range of the player.
   void setAudioEnabled(bool audio_enabled) override;

   /// \brief applies the distance-scaled volume to the sample that is currently playing.
   /// \param volume volume computed by the volume updater.
   void setVolume(float volume) override;

   /// \brief returns the wind area rectangle in pixel space.
   /// \return rectangular area used for force application checks.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief returns the normalized wind direction.
   /// \return unit vector the wind blows along, or a zero vector when no direction was configured.
   const sf::Vector2f& getDirection() const;

   /// \brief returns the force multiplier applied along the wind direction.
   /// \return configured strength.
   float getStrength() const;

   /// \brief creates a wind instance from tmx area and direction properties.
   /// \param parent owning game node in the scene graph.
   /// \param data deserialization data with bounds and direction components.
   /// \return configured wind mechanism.
   static std::shared_ptr<Wind> deserialize(GameNode* parent, const GameDeserializeData& data);

private:
   /// \brief keeps the sample playback going and re-reads the loudness the gusts are derived from.
   /// \param dt elapsed frame time.
   void updateSound(const sf::Time& dt);

   /// \brief turns the loudness of the currently playing sample into the force and leaf multipliers.
   /// \param loudness normalized loudness in 0..1, or 1 when no sample is audible.
   void applyLoudness(float loudness);

   /// \brief moves all leaves along the wind direction and recycles those that left the area.
   /// \param dt elapsed frame time.
   void updateLeaves(const sf::Time& dt);

   /// \brief creates the leaf instances and their sprites.
   void initializeLeaves();

   /// \brief randomizes one leaf and places it inside the area or on its upwind border.
   /// \param leaf leaf to reset.
   /// \param inside_area true to place the leaf anywhere inside the area, false to place it on the
   ///        border the wind blows in from.
   void respawnLeaf(Leaf& leaf, bool inside_area) const;

   sf::Vector2f _direction{0.0f, 0.0f};         //!< normalized wind direction, positive y points upward
   sf::Vector2f _direction_screen{0.0f, 0.0f};  //!< _direction with y flipped, so it can be applied to pixel positions
   float _strength{0.0f};                       //!< force multiplier applied along _direction
   sf::FloatRect _area;

   // leaves
   LeafSettings _leaf_settings;
   std::vector<Leaf> _leaves;
   std::shared_ptr<sf::Texture> _leaf_texture;
   int32_t _leaf_frame_count{1};

   // audio
   SoundRotation _sounds;
   float _sound_strength_influence{0.0f};  //!< 0 keeps the strength constant, 1 makes it follow the sample loudness
   float _strength_factor{1.0f};           //!< loudness-derived multiplier applied to _strength
   float _leaf_factor{1.0f};               //!< loudness-derived multiplier applied to the leaf velocity
};
