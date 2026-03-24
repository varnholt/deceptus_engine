#pragma once

#include <SFML/Graphics.hpp>

#include "game/constants.h"

#include <chrono>
#include <memory>
#include <vector>

/// \brief drawable sprite-sheet animation that advances frames over time and can own child animations.
class Animation : public sf::Drawable, public sf::Transformable
{
public:
   using HighResDuration = std::chrono::high_resolution_clock::duration;

   /// \brief constructs an empty animation without frame data.
   Animation() = default;
   /// \brief copies animation state, frame data, textures, and current transform.
   /// \param anim animation instance to copy from.
   Animation(const Animation& anim);

   /// \brief draws the current frame to a single render target using the color texture.
   /// \param target render target that receives the animation geometry.
   /// \param states render states applied while drawing.
   void draw(sf::RenderTarget& target, sf::RenderStates states = {}) const override;
   /// \brief draws the current frame to color and normal targets for deferred-style lighting.
   /// \param color render target for the color texture.
   /// \param normal render target for the normal map texture when available.
   /// \param states render states applied while drawing.
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal, sf::RenderStates states = {}) const;
   /// \brief draws this animation and its child animations to a single render target.
   /// \param target render target that receives the animation geometry.
   /// \param states render states applied while drawing.
   void drawTree(sf::RenderTarget& target, sf::RenderStates states = {}) const;
   /// \brief draws this animation and its child animations to color and normal targets.
   /// \param color render target for the color texture.
   /// \param normal render target for the normal map texture when available.
   /// \param states render states applied while drawing.
   void drawTree(sf::RenderTarget& color, sf::RenderTarget& normal, sf::RenderStates states = {}) const;

   /// \brief advances the current frame based on elapsed time and playback settings.
   /// \param dt elapsed frame time since the previous update.
   void update(const sf::Time& dt);
   /// \brief resumes playback from the current frame.
   void play();
   /// \brief pauses playback without changing the current frame.
   void pause();
   /// \brief pauses playback and seeks back to the first frame.
   void stop();
   /// \brief seeks to the first frame and refreshes quad vertices.
   void seekToStart();

   /// \brief updates this animation and directly attached child animations.
   /// \param dt elapsed frame time since the previous update.
   void updateTree(const sf::Time& dt);
   /// \brief resumes playback for this animation and all direct children.
   void playTree();
   /// \brief pauses playback for this animation and all direct children.
   void pauseTree();
   /// \brief stops this animation and all direct children, then seeks each to frame zero.
   void stopTree();
   /// \brief seeks this animation and all direct children to their first frame.
   void seekToStartTree();

   /// \brief rebuilds quad positions and texture coordinates for the current frame rectangle.
   /// \param reset_time true to reset frame-local elapsed time to zero after updating vertices.
   void updateVertices(bool reset_time = true);

   /// \brief applies one tint color to all quad vertices.
   /// \param color vertex color used when rendering.
   void setColor(const sf::Color& color);
   /// \brief applies one tint color to this animation and all direct children.
   /// \param color vertex color used when rendering.
   void setColorTree(const sf::Color& color);
   /// \brief sets the alpha channel for all quad vertices and stores it as current opacity.
   /// \param alpha opacity in the range 0-255.
   void setAlpha(uint8_t alpha);
   /// \brief sets alpha for this animation and all direct children.
   /// \param alpha opacity in the range 0-255.
   void setAlphaTree(uint8_t alpha);
   /// \brief reports whether drawing is currently enabled for this animation.
   /// \return true when draw calls render this animation.
   bool isVisible() const;
   /// \brief enables or disables rendering for this animation.
   /// \param newVisible true to render the animation, false to skip drawing.
   void setVisible(bool newVisible);

   /// \brief computes local-space bounds from the current frame rectangle size.
   /// \return local bounds in pixels before transform is applied.
   sf::FloatRect getLocalBounds() const;
   /// \brief computes world-space bounds by transforming local bounds with the current transform.
   /// \return transformed bounds in world coordinates.
   sf::FloatRect getGlobalBounds() const;

   /// \brief replaces per-frame durations and recomputes total animation duration caches.
   /// \param frame_times duration list, one entry per frame.
   void setFrameTimes(const std::vector<sf::Time>& frame_times);
   /// \brief returns how many frame durations are configured for playback.
   /// \return number of timed frames in this animation.
   size_t getFrameCount() const;
   /// \brief reverses frame order and corresponding frame durations for backward playback.
   void reverse();

   /// \brief attaches a child animation that can be updated and drawn together with this one.
   /// \param child child animation instance to append.
   void addChild(const std::shared_ptr<Animation>& child);

   std::string _name;
   std::vector<sf::IntRect> _frames;
   std::shared_ptr<sf::Texture> _color_texture;
   std::shared_ptr<sf::Texture> _normal_texture;
   sf::Vertex _vertices[4];

   sf::Time _current_time;
   sf::Time _elapsed;
   sf::Time _overall_time;
   HighResDuration _overall_time_chrono;
   int32_t _current_frame{0};
   int32_t _previous_frame{-1};
   int32_t _loop_count{0};

   bool _paused{false};
   bool _looped{false};
   bool _reset_to_first_frame{true};
   bool _finished{false};
   bool _visible{true};

   std::vector<std::shared_ptr<Animation>> _children;
   uint8_t _alpha{255};

   /// \brief returns the per-frame duration table used by playback.
   /// \return const reference to the frame duration vector.
   const std::vector<sf::Time>& getFrameTimes() const;

private:
   std::vector<sf::Time> _frame_times;
};
