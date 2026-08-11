#pragma once

#include "game/io/gamedeserializedata.h"
#include "game/mechanisms/rope.h"

/// \brief a rope the player can grab, climb and swing on.
/// \note the chain, the drawing and the wind all come from Rope. what this mechanism adds is a chain
///       that holds its shape well enough to be hung from, and access to its links so the player side
///       can hang himself off one of them. everything about grabbing, climbing and swinging lives in
///       PlayerRope - a mechanism reading player input is what this deliberately avoids.
class GrabRope : public Rope
{
public:
   /// \brief creates a grab rope mechanism instance.
   /// \param parent owning game node in the scene graph.
   GrabRope(GameNode* parent);

   /// \brief returns the mechanism type name used by the serialization system.
   /// \return constant string view containing "GrabRope".
   std::string_view objectName() const override;

   /// \brief builds the rope chain, conditions it to be hung from and makes it collide with the level.
   /// \param data deserialization data with polyline path, world, and properties.
   void setup(const GameDeserializeData& data) override;

   /// \brief returns the chain bodies the player can hang on, ordered from the anchor downwards.
   /// \return chain element bodies.
   const std::vector<b2Body*>& getChainElements() const;

   /// \brief returns the distance between two neighbouring chain elements.
   /// \return segment length in box2d units.
   float getSegmentLength() const;

   /// \brief returns the rope's suspension point in the anchor body's local frame.
   /// \return suspension point in box2d units.
   /// \note Rope leaves its anchor body at the world origin and puts the suspension point into the
   ///       shape instead, so the body's local frame and the world frame are the same thing here.
   b2Vec2 getAnchorLocalPosition() const;

   /// \brief reports whether a rectangle is close enough to the chain to grab it.
   /// \param rect_px rectangle to test, in pixels.
   /// \return true when any segment of the chain passes through the rectangle.
   /// \note tested segment by segment rather than against one box around the whole chain: a rope swinging
   ///       wide has a box as wide as its swing, and a corner of that box can be nowhere near the rope.
   ///       the chain positions are used directly rather than Rope::getBoundingBoxPx so this follows the
   ///       rope while it moves.
   bool isWithinGrabRange(const sf::FloatRect& rect_px) const;

private:
   /// \brief describes every chain link as a rod hanging from its own pivot.
   /// \note Rope gives every link the same fixed tiny collision box regardless of how far apart the
   ///       joints put them, which leaves each link's inertia and centre of mass unrelated to the joint
   ///       arm it swings on. a short decorative rope survives that; a long one drifts apart on its own
   ///       and folds its slack into knots.
   void makeChainCarryable();
};
