#include "animation.h"

#include <iostream>
#include <numeric>

Animation::Animation(const Animation& anim)
    : sf::Drawable(anim),
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

void Animation::play()
{
   _paused = false;
}

void Animation::pause()
{
   _paused = true;
}

void Animation::seekToStart()
{
   _previous_frame = -1;
   _current_frame = 0;

   updateVertices();
}

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

void Animation::stop()
{
   _paused = true;
   seekToStart();
}

void Animation::playTree()
{
   play();
   for (const auto& child : _children)
   {
      child->play();
   }
}

void Animation::pauseTree()
{
   pause();
   for (const auto& child : _children)
   {
      child->pause();
   }
}

void Animation::stopTree()
{
   stop();
   for (const auto& child : _children)
   {
      child->stop();
   }
}

void Animation::seekToStartTree()
{
   seekToStart();
   for (const auto& child : _children)
   {
      child->seekToStart();
   }
}

void Animation::addChild(const std::shared_ptr<Animation>& child)
{
   _children.push_back(child);
}

const std::vector<sf::Time>& Animation::getFrameTimes() const
{
   return _frame_times;
}

bool Animation::isVisible() const
{
   return _visible;
}

void Animation::setVisible(bool visible)
{
   _visible = visible;
}

void Animation::setAlpha(uint8_t alpha)
{
   _vertices[0].color.a = alpha;
   _vertices[1].color.a = alpha;
   _vertices[2].color.a = alpha;
   _vertices[3].color.a = alpha;
}

void Animation::setAlphaTree(uint8_t alpha)
{
   setAlpha(alpha);
   for (const auto& child : _children)
   {
      child->setAlpha(alpha);
   }
}

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

   const auto& frame_time = _frame_times[static_cast<size_t>(_current_frame)];

   // if current time is bigger then the frame time advance one frame
   if (_current_time >= frame_time)
   {
      // reset time, but keep the remainder
      // clang-format off
      _current_time = sf::microseconds(
         (frame_time.asMicroseconds() > 0)
            ? _current_time.asMicroseconds() % frame_time.asMicroseconds()
            : _current_time.asMicroseconds()
      );
      // clang-format on

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

         if (_looped)
         {
            _loop_count++;
         }
         else
         {
            _paused = true;
         }
      }

      updateVertices(false);
   }

   _elapsed += dt;
}

void Animation::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
   if (!_visible)
   {
      return;
   }

   states.transform *= getTransform();
   states.texture = _color_texture.get();

   target.draw(_vertices, 6, sf::PrimitiveType::Triangles, states);

   for (const auto& child : _children)
   {
      child->draw(target, states);
   }
}

void Animation::draw(sf::RenderTarget& color, sf::RenderTarget& normal, sf::RenderStates states) const
{
   if (!_visible)
   {
      return;
   }

   states.transform *= getTransform();

   states.texture = _color_texture.get();
   color.draw(_vertices, 6, sf::PrimitiveType::Triangles, states);

   if (_normal_texture)
   {
      states.texture = _normal_texture.get();
      normal.draw(_vertices, 6, sf::PrimitiveType::Triangles, states);
   }
}

void Animation::drawTree(sf::RenderTarget& target, sf::RenderStates states) const
{
   if (!_visible)
   {
      return;
   }

   draw(target, states);
   for (const auto& child : _children)
   {
      child->draw(target, states);
   }
}

void Animation::drawTree(sf::RenderTarget& color, sf::RenderTarget& normal, sf::RenderStates states) const
{
   if (!_visible)
   {
      return;
   }

   draw(color, normal, states);
   for (const auto& child : _children)
   {
      child->draw(color, normal, states);
   }
}

void Animation::updateVertices(bool reset_time)
{
   const auto& rect_px = _frames[static_cast<size_t>(_current_frame)];

   const auto left = static_cast<float>(rect_px.position.x) + 0.0001f;
   const auto right = left + static_cast<float>(rect_px.size.x);
   const auto top = static_cast<float>(rect_px.position.y);
   const auto bottom = top + static_cast<float>(rect_px.size.y);

   sf::Vector2f top_left = {0.f, 0.f};
   sf::Vector2f bottom_left = {0.f, static_cast<float>(rect_px.size.y)};
   sf::Vector2f bottom_right = {static_cast<float>(rect_px.size.x), static_cast<float>(rect_px.size.y)};
   sf::Vector2f top_right = {static_cast<float>(rect_px.size.x), 0.f};

   _vertices[0] = sf::Vertex(top_left, sf::Color::White);
   _vertices[1] = sf::Vertex(bottom_left, sf::Color::White);
   _vertices[2] = sf::Vertex(bottom_right, sf::Color::White);

   _vertices[3] = sf::Vertex(top_left, sf::Color::White);
   _vertices[4] = sf::Vertex(bottom_right, sf::Color::White);
   _vertices[5] = sf::Vertex(top_right, sf::Color::White);

   sf::Vector2f tex_top_left = {left, top};
   sf::Vector2f tex_bottom_left = {left, bottom};
   sf::Vector2f tex_bottom_right = {right, bottom};
   sf::Vector2f tex_top_right = {right, top};

   _vertices[0].texCoords = tex_top_left;
   _vertices[1].texCoords = tex_bottom_left;
   _vertices[2].texCoords = tex_bottom_right;

   _vertices[3].texCoords = tex_top_left;
   _vertices[4].texCoords = tex_bottom_right;
   _vertices[5].texCoords = tex_top_right;

   if (reset_time)
   {
      _current_time = sf::Time::Zero;
   }
}

void Animation::setColor(const sf::Color& color)
{
   for (auto& vertex : _vertices)
   {
      vertex.color = color;
   }
}

void Animation::setColorTree(const sf::Color& color)
{
   setColor(color);
   for (const auto& child : _children)
   {
      child->setColor(color);
   }
}

sf::FloatRect Animation::getLocalBounds() const
{
   const sf::IntRect rect = _frames[static_cast<size_t>(_current_frame)];
   return sf::FloatRect({0.0f, 0.0f}, {static_cast<float>(std::abs(rect.size.x)), static_cast<float>(std::abs(rect.size.y))});
}

sf::FloatRect Animation::getGlobalBounds() const
{
   return getTransform().transformRect(getLocalBounds());
}

void Animation::setFrameTimes(const std::vector<sf::Time>& frame_times)
{
   _frame_times = frame_times;
   _overall_time = std::accumulate(frame_times.begin(), frame_times.end(), sf::Time{});
   _overall_time_chrono = std::chrono::milliseconds(_overall_time.asMilliseconds());
}

size_t Animation::getFrameCount() const
{
   return _frame_times.size();
}

void Animation::reverse()
{
   std::reverse(_frame_times.begin(), _frame_times.end());
   std::reverse(_frames.begin(), _frames.end());
}
