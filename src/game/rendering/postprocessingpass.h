#pragma once

#include <SFML/Graphics.hpp>

#include <memory>

/// \brief owns the render target and the blits needed to apply a post processing effect to a frame.
///
/// The frame-wide scope needs nothing but a shader on the final blit to the window. The level scope
/// does: a shader cannot read and write the same target, so the level has to land somewhere before
/// being resolved into the frame target. This class owns that intermediate target, creates it on
/// first use and releases it again, so a session that never selects the level scope never pays for
/// it.
///
/// Which effect applies and whether one applies at all is decided by PostProcessing; this class only
/// deals with where the pixels go.
class PostProcessingPass
{
public:
   /// \brief picks the target the level should render into this frame.
   ///
   /// Call once per frame before drawing the level, then hand the result to Level::draw.
   /// \param frame_target target the composited frame is assembled in.
   /// \param level_loaded whether a level is available to draw.
   /// \return the intermediate target while a level-scoped effect is active, the frame target otherwise.
   std::shared_ptr<sf::RenderTexture> selectLevelTarget(const std::shared_ptr<sf::RenderTexture>& frame_target, bool level_loaded);

   /// \brief resolves the intermediate target into the frame target through the effect.
   ///
   /// Call once per frame after drawing the level and before any overlay is drawn, so the hud ends
   /// up on top of the processed level. Does nothing when the level rendered straight into the
   /// frame target.
   /// \param frame_target target the composited frame is assembled in.
   /// \param view_to_texture_scale scale Level::draw uses for its own blit.
   void resolveLevelTarget(sf::RenderTexture& frame_target, float view_to_texture_scale);

   /// \brief returns the shader for the final blit of the composited frame to the window.
   /// \param frame_texture texture holding the composited frame.
   /// \return shader to draw with, or nullptr when nothing applies at this stage.
   const sf::Shader* getFrameShader(const sf::Texture& frame_texture);

   /// \brief releases the intermediate target, for a level change or a resolution change.
   void release();

private:
   /// \brief creates the intermediate target on first use.
   /// \param size size to match, taken from the frame target.
   /// \return true when the target is available.
   bool createRenderTexture(const sf::Vector2u& size);

   //! \brief intermediate target the level renders into while a level-scoped effect is active
   std::shared_ptr<sf::RenderTexture> _render_texture;

   //! \brief whether this frame routes the level through the intermediate target
   bool _level_scope_active{false};
};
