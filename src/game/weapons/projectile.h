#pragma once

#include "box2d/box2d.h"
#include <SFML/Graphics.hpp>

#include "game/audio/audioupdatedata.h"
#include "game/level/fixturenode.h"
#include "game/level/gamenode.h"
#include "game/weapons/projectilehitanimation.h"

#include <functional>
#include <list>
#include <map>
#include <set>

class b2Body;

/// \brief base class for live projectile entities with physics, animation, and hit effects.
class Projectile : public FixtureNode
{
public:
   /// \brief callback invoked when a projectile is destroyed.
   using DestroyedCallback = std::function<void(void)>;

   /// \brief registers this projectile in the global projectile set and ensures default hit animation data exists.
   Projectile();
   /// \brief notifies destruction callbacks and destroys the associated box2d body when present.
   virtual ~Projectile();

   /// \brief returns the box2d body backing this projectile.
   /// \return pointer to the projectile body, or nullptr when not assigned.
   b2Body* getBody() const;
   /// \brief assigns the box2d body used by this projectile.
   /// \param body projectile physics body pointer.
   void setBody(b2Body* body);

   /// \brief deletes all registered projectiles and clears global projectile state.
   static void clear();
   /// \brief updates projectile lifetime, handles removals, and advances hit animations.
   /// \param dt frame delta time.
   static void update(const sf::Time& dt);
   /// \brief returns the global set of currently tracked projectiles.
   /// \return reference to the static projectile set.
   static std::set<Projectile*>& getProjectiles();

   /// \brief registers a callback executed during projectile destruction.
   /// \param destroyedCallback callback to invoke before the body is destroyed.
   void addDestroyedCallback(const DestroyedCallback& destroyedCallback);

   /// \brief checks whether the projectile should stick to impacted objects.
   /// \return true when sticky behavior is enabled.
   bool isSticky() const;
   /// \brief enables or disables sticky behavior.
   /// \param sticky true to keep projectile attached on impact.
   void setSticky(bool sticky);

   /// \brief checks whether this projectile already registered an impact.
   /// \return true when an impact was reported.
   bool hitSomething() const;
   /// \brief marks whether this projectile has impacted something.
   /// \param hit_something true when the projectile has collided with a valid target.
   void setHitSomething(bool hit_something);

   /// \brief checks whether this projectile is queued for deletion.
   /// \return true when the projectile will be deleted in the update flow.
   bool isScheduledForRemoval() const;
   /// \brief marks this projectile for removal in the next collection step.
   /// \param isScheduledForRemoval true to schedule deletion.
   void setScheduledForRemoval(bool isScheduledForRemoval);

   /// \brief checks whether this projectile should be disabled instead of removed.
   /// \return true when inactive scheduling is enabled.
   bool isScheduledForInactivity() const;
   /// \brief marks this projectile to have its body disabled by weapon update logic.
   /// \param scheduled_for_inactivity true to schedule body disable.
   void setScheduledForInactivity(bool scheduled_for_inactivity);

   /// \brief checks whether animation rotation follows projectile rotation.
   /// \return true when rotating visuals are enabled.
   bool isRotating() const;
   /// \brief enables or disables rotation-driven animation rendering.
   /// \param rotating true to apply stored rotation to animation.
   void setRotating(bool rotating);

   /// \brief returns the projectile rotation in radians.
   /// \return current rotation value used for rendering.
   float getRotation() const;
   /// \brief sets the projectile rotation used by rendering and hit effects.
   /// \param rotation rotation angle in radians.
   void setRotation(float rotation);

   /// \brief returns the animation instance used to draw this projectile.
   /// \return mutable reference to projectile animation.
   Animation& getAnimation();
   /// \brief replaces the projectile animation state.
   /// \param sprite animation template copied into this projectile.
   void setAnimation(const Animation& sprite);

   /// \brief returns the identifier used for hit animation and audio lookup.
   /// \return projectile effect identifier string.
   std::string getProjectileIdentifier() const;
   /// \brief sets the identifier used for hit animation and audio lookup.
   /// \param projectile_identifier effect identifier key.
   void setProjectileIdentifier(const std::string& projectile_identifier);

   /// \brief stores optional parent audio context propagated from the owning weapon.
   /// \param audio_update_data audio update context copied into the projectile.
   void setParentAudioUpdateData(const AudioUpdateData& audio_update_data);

   /// \brief returns optional parent audio context inherited from the owning weapon.
   /// \return optional audio update data when set.
   std::optional<AudioUpdateData> getParentAudioUpdateData() const;
   /// \brief enables or disables hit sound playback for this projectile.
   /// \param enabled true to allow hit audio playback.
   void setAudioEnabled(bool enabled);

protected:
   /// \brief gathers hit data from projectiles scheduled for removal and deletes them.
   static void collectHitInformation();
   /// \brief spawns hit animations and optionally plays hit audio for collected impacts.
   static void processHitInformation();

   static constexpr auto default_projectile_identifier = "default";

   bool _scheduled_for_removal = false;
   bool _scheduled_for_inactivity = false;
   bool _sticky = false;
   bool _hit_something = false;
   bool _rotating = false;
   float _rotation = 0.0f;
   b2Body* _body = nullptr;
   WeaponType _weapon_type = WeaponType::None;
   std::string _projectile_identifier = default_projectile_identifier;
   std::vector<DestroyedCallback> _destroyed_callbacks;
   sf::Time _time_alive;

   Animation _animation;
   sf::Rect<int32_t> _animation_texture_rect;

   std::optional<AudioUpdateData> _parent_audio_update_data;
   bool _audio_enabled{true};
};
