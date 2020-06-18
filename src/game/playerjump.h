#pragma once

#include "playercontrols.h"

#include <SFML/Graphics.hpp>

#include <functional>

class b2Body;
class b2Joint;

struct PlayerJump
{
    PlayerJump() = default;

    void jump();
    void jumpImpulse(b2Body*);
    void jumpForce();

    void update(b2Body* body, bool inAir, bool inWater, bool crouching, bool climbing, const PlayerControls& controls);

    void updateJumpBuffer();
    void updateJump(b2Body*);
    void updateLostGroundContact();

    bool isJumping() const;

    sf::Clock mJumpClock;
    sf::Time mLastJumpPressTime;
    sf::Time mGroundContactLostTime;

    int32_t mJumpSteps = 0;

    bool mHadGroundContact = true;
    bool mGroundContactJustLost = false;

    bool mInAir = false;
    bool mInWater = false;
    bool mCrouching = false;
    bool mJumpButtonPressed = false;
    bool mClimbing = false;

    std::function<void(void)> mDustAnimation;
    std::function<void(void)> mRemoveClimbJoint;
};

