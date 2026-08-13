#ifndef PLAYERROPEHOLD_H
#define PLAYERROPEHOLD_H

#include "box2d/box2d.h"
#include "game/player/playercontrols.h"

#include <memory>
#include <optional>

/// \brief the player hanging on a rope: the joint that carries him, the swing control and the key ownership.
/// \note shared by the harpoon, whose rope is built and torn down on the fly, and by the grab rope
///       mechanism, whose chain already exists in the level. both hang the player off one single rope
///       body through a distance joint whose length can be changed every frame, both let him swing by
///       pushing along the arc, and both own up and down while he hangs.
class PlayerRopeHold
{
public:
   /// \brief hangs the player off a rope body.
   /// \param world physics world the joint is created in.
   /// \param rope_body rope body carrying the player.
   /// \param player_body player body to hang.
   /// \param local_anchor_rope attachment point on the rope body, in its local frame.
   /// \param link_length_m length of the link between rope body and player.
   void attach(
      const std::shared_ptr<b2World>& world,
      b2Body* rope_body,
      b2Body* player_body,
      const b2Vec2& local_anchor_rope,
      float link_length_m
   );

   /// \brief destroys the joint that carries the player.
   /// \note the vertical key claim is not touched here, it is driven by syncVerticalKeyClaim alone.
   void detach();

   /// \brief moves the hold onto another rope body without letting go of the player.
   /// \param rope_body rope body that carries the player from now on.
   /// \param local_anchor_rope attachment point on the new rope body, in its local frame.
   /// \param link_length_m length of the link between the new rope body and the player.
   void rebind(b2Body* rope_body, const b2Vec2& local_anchor_rope, float link_length_m);

   /// \brief changes the link length without recreating the joint.
   /// \param link_length_m new length of the link.
   void setLinkLength(float link_length_m);

   /// \brief limits how far the player can get from the point the rope hangs from.
   /// \param anchor_body body the rope is suspended from.
   /// \param local_anchor suspension point in that body's local frame.
   /// \param max_distance_m amount of rope between the suspension point and the player.
   /// \note a chain of revolute joints is only as rigid as the solver makes it, and a long one carrying
   ///       a body as heavy as the player stretches visibly - far enough to set him down on the floor
   ///       below. this second link is a hard ceiling on that: it never pulls, it only refuses to let
   ///       him travel further from the suspension point than there is rope between them. the chain
   ///       stays free to sag and swing, it just cannot be pulled apart any more.
   void setAnchorLimit(b2Body* anchor_body, const b2Vec2& local_anchor, float max_distance_m);

   /// \brief updates the maximum distance of an existing anchor limit.
   /// \param max_distance_m amount of rope between the suspension point and the player.
   void setAnchorLimitDistance(float max_distance_m);

   /// \brief returns the current link length.
   /// \return length of the link between rope body and player.
   float getLinkLength() const;

   /// \brief returns the rope body the player currently hangs on.
   /// \return rope body, or nullptr while detached.
   b2Body* getRopeBody() const;

   /// \brief reports whether the player currently hangs on a rope.
   /// \return true while the joint exists.
   bool isAttached() const;

   /// \brief returns the world position where the link ends on the player, which is where the rope is drawn to.
   /// \return anchor position in box2d world units, or nullopt while detached.
   std::optional<b2Vec2> readPlayerAnchorPosition() const;

   /// \brief applies the reduced horizontal control the player keeps while swinging.
   /// \param player_body player body the force is applied to.
   /// \param direction requested direction, -1 points left and 1 points right.
   /// \param acceleration swing acceleration in box2d units.
   void applySwingControl(b2Body* player_body, float direction, float acceleration) const;

   /// \brief pulls the player towards a point on the rope so shortening the link lifts him.
   /// \param player_body player body the force is applied to.
   /// \param target_m world position to pull towards.
   /// \param acceleration pull acceleration in box2d units.
   void pullPlayerTowards(b2Body* player_body, const b2Vec2& target_m, float acceleration) const;

   /// \brief takes up and down away from the rest of the game while the player owns them.
   /// \param controls controls the keys are claimed on.
   /// \param owns_vertical_keys true while up and down belong to the rope rather than to the game.
   void syncVerticalKeyClaim(const std::shared_ptr<PlayerControls>& controls, bool owns_vertical_keys);

   /// \brief reports whether this hold currently owns up and down.
   /// \return true while the claim is held.
   bool ownsVerticalKeys() const;

   /// \brief reports whether up and down already belong to a different owner.
   /// \param controls controls to query.
   /// \return true when the keys are claimed and this hold is not the one holding them.
   /// \note this is what keeps a second feature from becoming a second owner of the same key. the claim
   ///       stops non-owners from reading a key, it cannot decide which of two owners should win - so
   ///       whoever wants to become one asks first, and first come wins.
   bool areVerticalKeysOwnedElsewhere(const std::shared_ptr<PlayerControls>& controls) const;

   /// \brief reads the vertical input axis, which only the owner of those keys may do.
   /// \return normalized vertical input in -1 to 1, or 0 while this hold does not own the keys.
   float readVerticalAxis() const;

   /// \brief reads the horizontal controller axis, paired with the vertical one for analogue aiming.
   /// \return normalized horizontal input in -1 to 1, or 0 while this hold does not own the keys.
   float readHorizontalAxis() const;

   /// \brief reports whether the owner's vertical input points up.
   /// \return true when the axis is past the deadzone and negative.
   bool isUpPressed() const;

   /// \brief reports whether the owner's vertical input points down.
   /// \return true when the axis is past the deadzone and positive.
   bool isDownPressed() const;

   /// \brief returns the world position the rope is attached to on the player.
   /// \param player_body player body the rope end is jointed to.
   /// \return rope attachment position in box2d world units.
   static b2Vec2 readPlayerAttachmentPosition(b2Body* player_body);

private:
   /// \brief creates the distance joint that carries the player.
   void createJoint();

   /// \brief destroys the distance joint if one exists.
   void destroyJoint();

   /// \brief destroys the anchor limit joint if one exists.
   void destroyAnchorLimit();

   std::shared_ptr<b2World> _world;
   b2Body* _rope_body{nullptr};
   b2Body* _player_body{nullptr};
   b2DistanceJoint* _joint{nullptr};
   b2DistanceJoint* _anchor_limit_joint{nullptr};  //!< keeps a soft chain from being stretched apart
   b2Vec2 _local_anchor_rope{0.0f, 0.0f};
   float _link_length_m{0.0f};
   KeyClaim _vertical_key_claim;
};

#endif  // PLAYERROPEHOLD_H
