#ifndef SKILLGATE_H
#define SKILLGATE_H

#include <memory>
#include <optional>

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"
#include "game/player/skill.h"

/// \brief represents a solid rectangle that only lets the player pass once a given player skill is unlocked.
/// the gate reads its condition from the player skill bitmask every frame, so it opens as soon as the skill is
/// granted by a level script, an extra, or the debug console. no state is serialized because the unlocked skills
/// are already part of the save state.
class SkillGate : public GameMechanism, public GameNode
{
public:
   /// \brief creates a skill gate node.
   /// \param parent parent node in the scene graph.
   SkillGate(GameNode* parent = nullptr);

   /// \brief returns the mechanism type identifier.
   /// \return non-owning string view with value "SkillGate".
   std::string_view objectName() const override;

   /// \brief initializes rectangle geometry, required skill, optional textures, and a static box2d collider from tmx data.
   /// \param data deserialize context containing object properties and physics world.
   void setup(const GameDeserializeData& data);

   /// \brief draws the configured texture and normal map with the current fade alpha applied.
   /// \param target color render target.
   /// \param normal normal-map render target.
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;

   /// \brief draws the configured texture and normal map with explicit render states (used in WASM to carry the level view).
   /// \param target color render target.
   /// \param normal normal-map render target.
   /// \param states render states to apply.
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal, const sf::RenderStates& states) override;
   using GameMechanism::draw;

   /// \brief reconciles collider state with the player skill set and advances the fade animation.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief returns the gate rectangle in pixel coordinates.
   /// \return rectangle bounds used for culling and overlap checks.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief returns the configured pixel rectangle.
   /// \return rectangle occupied by this gate.
   const sf::FloatRect& getPixelRect() const;

   /// \brief checks whether the player may currently pass this gate.
   /// \return true when the skill condition is satisfied.
   bool isPassable() const;

private:
   /// \brief checks whether the gate should currently be solid.
   /// \return true when the mechanism is enabled and the skill condition is not satisfied.
   bool isBlocking() const;

   /// \brief applies the blocking state to the box2d body.
   /// \param blocking true to make the gate collidable.
   void applyBlockingState(bool blocking);

   std::optional<Skill::SkillType> _required_skill;  //!< skill that opens this gate, unset when the tmx property was invalid
   bool _inverted{false};    //!< when true the gate blocks while the skill is unlocked instead of while it is missing
   float _alpha{1.0f};       //!< current fade value, 1 when fully solid and 0 when fully dissolved
   float _fade_speed{2.0f};  //!< alpha change per second while fading in or out

   // rendering
   std::shared_ptr<sf::Texture> _texture_map;
   std::shared_ptr<sf::Texture> _normal_map;
   std::unique_ptr<sf::Sprite> _sprite;
   sf::FloatRect _rectangle;

   // physics
   b2Body* _body = nullptr;
   b2PolygonShape _shape_bounds;
};

#endif  // SKILLGATE_H
