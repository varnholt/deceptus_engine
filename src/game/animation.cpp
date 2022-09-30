#include "animation.h"

#include <iostream>
#include <numeric>



//----------------------------------------------------------------------------------------------------------------------
Animation::Animation(const Animation& anim)
    : sf::Sprite(anim),
      _name(anim._name),
      _frames(anim._frames),
      _color_texture(anim._color_texture),
      _normal_texture(anim._normal_texture),
      _overall_time(anim._overall_time),
      _overall_time_chrono(anim._overall_time_chrono),
      _frame_times(anim._frame_times)
{
   setOrigin(anim.getOrigin());
   setRotation(anim.getRotation());

   _vertices[0] = anim._vertices[0];
   _vertices[1] = anim._vertices[1];
   _vertices[2] = anim._vertices[2];
   _vertices[3] = anim._vertices[3];
}


//----------------------------------------------------------------------------------------------------------------------
void Animation::play()
{
   _paused = false;
}


//----------------------------------------------------------------------------------------------------------------------
void Animation::pause()
{
   _paused = true;
}


//----------------------------------------------------------------------------------------------------------------------
void Animation::seekToStart()
{
   _previous_frame = -1;
   _current_frame = 0;

   updateVertices();
}


//----------------------------------------------------------------------------------------------------------------------
void Animation::updateTree(const sf::Time& dt)
{
   if (_frame_times.empty())
   {
      return;
   }

   if (_paused)
   {
      return;
   }

   update(dt);
   for (const auto& child : _children)
   {
      child->update(dt);
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Animation::stop()
{
   _paused = true;
   seekToStart();
}

//----------------------------------------------------------------------------------------------------------------------
void Animation::playTree()
{
   play();
   for (const auto& child : _children)
   {
      child->play();
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Animation::pauseTree()
{
   pause();
   for (const auto& child : _children)
   {
      child->pause();
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Animation::stopTree()
{
   stop();
   for (const auto& child : _children)
   {
      child->stop();
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Animation::seekToStartTree()
{
   seekToStart();
   for (const auto& child : _children)
   {
      child->seekToStart();
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Animation::addChild(const std::shared_ptr<Animation>& child)
{
   _children.push_back(child);
}


//----------------------------------------------------------------------------------------------------------------------
void Animation::setAlpha(uint8_t alpha)
{
   _vertices[0].color.a = alpha;
   _vertices[1].color.a = alpha;
   _vertices[2].color.a = alpha;
   _vertices[3].color.a = alpha;
}


//----------------------------------------------------------------------------------------------------------------------
void Animation::setAlphaTree(uint8_t alpha)
{
   setAlpha(alpha);
   for (const auto& child : _children)
   {
      child->setAlpha(alpha);
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Animation::update(const sf::Time& dt)
{
   if (_frame_times.empty())
   {
      return;
   }

   if (_paused)
   {
      return;
   }

   _finished = false;
   _previous_frame = _current_frame;
   _current_time += dt;

   const auto& frameTime = _frame_times[static_cast<size_t>(_current_frame)];

   // if current time is bigger then the frame time advance one frame
   if (_current_time >= frameTime)
   {
      // reset time, but keep the remainder
      _current_time = sf::microseconds(
         (frameTime.asMicroseconds() > 0)
            ? _current_time.asMicroseconds() % frameTime.asMicroseconds()
            : _current_time.asMicroseconds()
      );

      if (_current_frame + 1 < static_cast<int32_t>(_frames.size()))
      {
         _current_frame++;
      }
      else
      {
         _finished = true;

         if (_reset_to_first_frame)
         {
            _current_frame = 0;
         }

         if (!_looped)
         {
            _paused = true;
         }
      }

      updateVertices(false);
   }

   _elapsed += dt;
}


//----------------------------------------------------------------------------------------------------------------------
void Animation::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
   states.transform *= getTransform();
   states.texture = _color_texture.get();

   target.draw(_vertices, 4, sf::Quads, states);

   for (const auto& child : _children)
   {
      child->draw(target, states);
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Animation::draw(sf::RenderTarget& color, sf::RenderTarget& normal, sf::RenderStates states) const
{
   states.transform *= getTransform();

   states.texture = _color_texture.get();
   color.draw(_vertices, 4, sf::Quads, states);

   if (_normal_texture)
   {
      states.texture = _normal_texture.get();
      normal.draw(_vertices, 4, sf::Quads, states);
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Animation::drawTree(sf::RenderTarget& target, sf::RenderStates states) const
{
   draw(target, states);
   for (const auto& child : _children)
   {
      child->draw(target, states);
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Animation::drawTree(sf::RenderTarget& color, sf::RenderTarget& normal, sf::RenderStates states) const
{
   draw(color, normal, states);
   for (const auto& child : _children)
   {
      child->draw(color, normal, states);
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Animation::updateVertices(bool resetTime)
{
   const auto& rect_px = _frames[static_cast<size_t>(_current_frame)];

   const auto l =     static_cast<float>(rect_px.left) + 0.0001f;
   const auto r = l + static_cast<float>(rect_px.width);
   const auto t =     static_cast<float>(rect_px.top);
   const auto b = t + static_cast<float>(rect_px.height);

   _vertices[0].position = sf::Vector2f(0.f, 0.f);
   _vertices[1].position = sf::Vector2f(0.f, static_cast<float>(rect_px.height));
   _vertices[2].position = sf::Vector2f(static_cast<float>(rect_px.width), static_cast<float>(rect_px.height));
   _vertices[3].position = sf::Vector2f(static_cast<float>(rect_px.width), 0.f);

   _vertices[0].texCoords = sf::Vector2f(l, t);
   _vertices[1].texCoords = sf::Vector2f(l, b);
   _vertices[2].texCoords = sf::Vector2f(r, b);
   _vertices[3].texCoords = sf::Vector2f(r, t);

   if (resetTime)
   {
      _current_time = sf::Time::Zero;
   }
}


//----------------------------------------------------------------------------------------------------------------------
sf::FloatRect Animation::getLocalBounds() const
{
   sf::IntRect rect = _frames[static_cast<size_t>(_current_frame)];
   return sf::FloatRect(
      0.f,
      0.f,
      static_cast<float>(std::abs(rect.width)),
      static_cast<float>(std::abs(rect.height))
   );
}


//----------------------------------------------------------------------------------------------------------------------
sf::FloatRect Animation::getGlobalBounds() const
{
   return getTransform().transformRect(getLocalBounds());
}


//----------------------------------------------------------------------------------------------------------------------
void Animation::setFrameTimes(const std::vector<sf::Time>& frame_times)
{
   _frame_times = frame_times;
   _overall_time = std::accumulate(frame_times.begin(), frame_times.end(), sf::Time{});
   _overall_time_chrono = std::chrono::milliseconds(_overall_time.asMilliseconds());
}


//----------------------------------------------------------------------------------------------------------------------
size_t Animation::getFrameCount() const
{
   return _frame_times.size();
}


//----------------------------------------------------------------------------------------------------------------------
void Animation::reverse()
{
   std::reverse(_frame_times.begin(), _frame_times.end());
   std::reverse(_frames.begin(), _frames.end());
}

