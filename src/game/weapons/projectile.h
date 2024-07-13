#pragma once

#include <box2d/box2d.h>
#include <SFML/Graphics.hpp>

#include "game/audio/audioupdatedata.h"
#include "game/fixturenode.h"
#include "game/gamenode.h"
#include "game/weapons/projectilehitanimation.h"

#include <functional>
#include <list>
#include <map>
#include <set>

class b2Body;

class Projectile : public FixtureNode
{
public:

   using DestroyedCallback = std::function<void(void)>;

   Projectile();
   virtual ~Projectile();

   b2Body* getBody() const;
   void setBody(b2Body* body);

   static void clear();
   static void update(const sf::Time& dt);
   static std::set<Projectile*>& getProjectiles();

   void addDestroyedCallback(const DestroyedCallback& destroyedCallback);

   bool isSticky() const;
   void setSticky(bool sticky);

   bool hitSomething() const;
   void setHitSomething(bool hit_something);

   bool isScheduledForRemoval() const;
   void setScheduledForRemoval(bool isScheduledForRemoval);

   bool isScheduledForInactivity() const;
   void setScheduledForInactivity(bool scheduled_for_inactivity);

   bool isRotating() const;
   void setRotating(bool rotating);

   float getRotation() const;
   void setRotation(float rotation);

   Animation& getAnimation();
   void setAnimation(const Animation& sprite);

   std::string getProjectileIdentifier() const;
   void setProjectileIdentifier(const std::string& projectile_identifier);

   void setParentAudioUpdateData(const AudioUpdateData& audio_update_data);

   std::optional<AudioUpdateData> getParentAudioUpdateData() const;
   void setAudioEnabled(bool enabled);

protected:
   static void collectHitInformation();
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
