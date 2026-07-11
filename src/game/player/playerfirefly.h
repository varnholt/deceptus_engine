
#pragma once

#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <SFML/Graphics.hpp>
#include <memory>

/// \brief a single firefly companion that orbits the player on a lemniscate (figure-8) path,
///        switching render depth between the layer in front of and behind the player.
class PlayerFirefly : public GameMechanism, public GameNode
{
public:
   /// \brief creates the firefly companion, loads the shared firefly texture, and sets initial depth.
   /// \param parent parent node in the scene graph.
   PlayerFirefly(GameNode* parent = nullptr);

   /// \brief returns the mechanism registry name.
   std::string_view objectName() const override;

   /// \brief draws the firefly sprite at its current position.
   /// \param target color render target.
   /// \param normal normal-map render target (unused).
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;

#ifdef __EMSCRIPTEN__
   /// \brief draws the firefly sprite with explicit render states (used in WASM to carry the level view).
   /// \param target color render target.
   /// \param normal normal-map render target (unused).
   /// \param states render states to apply.
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal, const sf::RenderStates& states) override;
   using GameMechanism::draw;
#endif

   /// \brief advances the lemniscate position, virtual-center follow, animation, and render depth.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief returns nullopt because this mechanism does not expose collision bounds.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief disabling also resets the virtual center so the next equip snaps to the player.
   /// \param enabled true to activate.
   void setEnabled(bool enabled) override;

private:
   /// \brief advances the texture-rect to the current animation frame.
   void updateTextureRect();

   sf::Time _elapsed;                          //!< accumulated time used for lemniscate phase and animation
   sf::Vector2f _virtual_center_px;            //!< lagging follow position used as the orbit center
   sf::Vector2f _previous_player_position_px;  //!< player position from the previous frame, used to compute movement delta
   sf::Vector2f _position_px;                  //!< current world-space position of the sprite
   float _speed{0.8f};                         //!< angular speed of the lemniscate traversal
   float _dir{1.0f};                           //!< traversal direction (+1 or -1)
   float _animation_speed{3.0f};               //!< animation frame rate in frames per second
   int32_t _current_frame{0};                  //!< last displayed animation frame index
   bool _initialized{false};                   //!< true after virtual center has been seeded from player position

   std::unique_ptr<sf::Sprite> _sprite;    //!< firefly sprite
   std::shared_ptr<sf::Texture> _texture;  //!< shared firefly texture
};
