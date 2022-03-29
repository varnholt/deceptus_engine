#include "animationplayer.h"


void AnimationPlayer::add(const std::shared_ptr<Animation>& animation)
{
   _animations.push_back(std::move(animation));
}


void AnimationPlayer::add(const std::vector<std::shared_ptr<Animation>>& animations)
{
   _animations.insert(_animations.end(), animations.begin(), animations.end());
}


void AnimationPlayer::update(const sf::Time& dt)
{
   for (auto& anim : _animations)
   {
      anim->update(dt);
   }

   // clear all elapsed animations
   _animations.erase(
      std::remove_if(
         _animations.begin(),
         _animations.end(),
         [](const auto& animation){return animation->_paused;}
      ),
      _animations.end()
   );
}


void AnimationPlayer::draw(sf::RenderTarget& target)
{
   for (auto& anim : _animations)
   {
      target.draw(*anim);
   }
}


AnimationPlayer& AnimationPlayer::getInstance()
{
   static AnimationPlayer _animation_player;
   return _animation_player;
}


