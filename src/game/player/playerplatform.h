#ifndef PLAYERPLATFORM_H
#define PLAYERPLATFORM_H

#include "box2d/box2d.h"
#include <SFML/Graphics.hpp>
#include <optional>

/// \brief tracks moving-platform state for the player body.
class PlayerPlatform
{
public:
   /// \brief creates a platform helper with default state.
   PlayerPlatform() = default;

   /// \brief applies platform motion and temporary gravity changes to the player body.
   /// \param body player box2d body to update.
   /// \param jumping true while the player is in a jump state; this restores cached gravity and skips platform snapping.
   void update(b2Body* body, bool jumping);

   /// \brief checks whether the player is grounded and in contact with a moving platform or death block.
   /// \return true when platform-following logic should be active.
   bool isOnPlatform() const;

   /// \brief checks whether the player foot sensor currently touches the ground.
   /// \return true when the player has at least one foot contact.
   bool isOnGround() const;

   /// \brief stores the platform body detected by contact handling.
   /// \param body platform body currently associated with the player.
   void setPlatformBody(b2Body* body);

   /// \brief returns the cached platform body pointer.
   /// \return platform body pointer, or nullptr when no platform is tracked.
   b2Body* getPlatformBody() const;

   /// \brief sets the horizontal offset applied to the player while riding a platform.
   /// \param dx_px platform delta-x in physics units for the current frame.
   void setPlatformDx(float dx_px);

   /// \brief sets the gravity scale used while the player is considered on a platform.
   /// \param scale gravity scale to apply to the player body during platform tracking.
   void setGravityScale(float scale);

   /// \brief clears cached platform contacts and restores internal temporary state.
   void reset();

private:
   b2Body* _platform_body = nullptr;
   float _platform_dx{0.0f};
   std::optional<float> _platform_gravity_scale;
   float _gravity_scale{10.0f};
};

#endif  // PLAYERPLATFORM_H
