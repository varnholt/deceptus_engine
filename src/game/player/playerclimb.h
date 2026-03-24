#pragma once

#include "box2d/box2d.h"
#include <functional>
#include "playercontrols.h"

class b2Joint;
class b2ChainShape;

/// \brief stores wall-climb joint state and related edge analysis.
struct PlayerClimb
{
   /// \brief constructs climb state with no active climb joint.
   PlayerClimb() = default;

   /// \brief runs wall-climb behavior for the current frame.
   /// \param body player body that would receive or release climb joints.
   /// \param in_air true when the player is airborne.
   void update(b2Body* body, bool in_air);
   /// \brief destroys the active climb joint if one exists.
   void removeClimbJoint();
   /// \brief determines whether a chain-shape vertex forms a climbable edge pattern.
   /// \param shape chain shape that contains the candidate edge.
   /// \param currIndex index of the current vertex inside the chain.
   /// \return true when neighboring vertices satisfy one of the climbable corner patterns.
   bool isClimbableEdge(b2ChainShape* shape, int currIndex);
   /// \brief determines whether the candidate edge lies in the direction requested by player input.
   /// \param edgeDir vector from the edge toward the player.
   /// \return true when the edge orientation matches pressed left or right input.
   bool edgeMatchesMovement(const b2Vec2& edgeDir);
   /// \brief reports whether a climb joint is currently active.
   /// \return true when the player is attached to geometry by a climb joint.
   bool isClimbing() const;

   b2Joint* _climb_joint = nullptr;
   int32_t _keys_pressed = 0;

   std::shared_ptr<PlayerControls> _controls;

public:
   /// \brief assigns the controls object used by climb edge matching.
   /// \param newControls shared controls instance owned by the player.
   void setControls(const std::shared_ptr<PlayerControls>& newControls);
};
