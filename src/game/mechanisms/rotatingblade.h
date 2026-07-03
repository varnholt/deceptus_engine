#pragma once

#include "framework/math/pathinterpolation.h"
#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <vector>
#include "SFML/Graphics.hpp"

/// \brief moves and spins a circular blade that damages the player on contact.
class RotatingBlade : public GameMechanism, public GameNode
{
public:
   /// \brief runtime tuning values for blade acceleration, rotation, and path speed.
   struct Settings
   {
      float _blade_acceleration = 0.006f;
      float _blade_deceleration = 0.009f;
      float _blade_rotation_speed = 400.0f;
      float _movement_speed = 0.2f;
   };

   /// \brief movement path interpretation mode taken from tmx geometry.
   enum class PathType
   {
      Polyline,
      Polygon
   };

   /// \brief initializes blade sprites, audio data, and default render order.
   /// \param parent owning game node in the scene graph.
   RotatingBlade(GameNode* parent = nullptr);

   /// \brief stops any active blade audio samples before destruction.
   ~RotatingBlade();

   /// \brief returns the mechanism type name used by the serialization system.
   /// \return constant string view containing "RotatingBlade".
   std::string_view objectName() const override;

   /// \brief reads path and properties from tmx data and configures blade behavior.
   /// \param data deserialization data with shape path and blade properties.
   void setup(const GameDeserializeData& data);

   /// \brief preloads blade movement audio samples.
   void preload() override;

   /// \brief advances blade speed, movement, rotation, audio state, and hit detection.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief draws the rotating blade sprite.
   /// \param target render target.
   /// \param normal normal-map render target, unused by this mechanism.
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;

   /// \brief draws the rotating blade sprite with explicit render states (used in WASM to carry the level view).
   /// \param target render target.
   /// \param normal normal-map render target, unused by this mechanism.
   /// \param states render states to apply.
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal, const sf::RenderStates& states) override;
   using GameMechanism::draw;

   /// \brief enables or disables blade sound playback updates.
   /// \param enabled true to allow blade audio.
   void setAudioEnabled(bool enabled) override;

   /// \brief updates base reference volume and applies it to active blade samples.
   /// \param volume target reference volume.
   void setReferenceVolume(float volume) override;

   /// \brief returns the mechanism bounds used for chunk activation.
   /// \return rectangle centered on the blade origin area.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief returns the stored pixel rectangle configured at setup time.
   /// \return blade rectangle in pixel space.
   const sf::FloatRect& getPixelRect() const;

private:
   /// \brief switches between accelerate, running, and decelerate audio loops.
   void updateAudio();

   float _angle = 0.0f;
   float _velocity = 0.0f;
   float _direction = 1.0f;

   std::shared_ptr<sf::Texture> _texture_map;
   std::shared_ptr<sf::Texture> _normal_map;
   std::unique_ptr<sf::Sprite> _sprite;
   sf::FloatRect _rectangle;
   std::vector<sf::Vector2f> _path;
   sf::Vector2f _pos;
   PathInterpolation<sf::Vector2f> _path_interpolation;
   PathType _path_type = PathType::Polygon;
   Settings _settings;
   std::optional<int32_t> _sample_enabled;
   std::optional<int32_t> _sample_accelerate;
   std::optional<int32_t> _sample_decelerate;
};
