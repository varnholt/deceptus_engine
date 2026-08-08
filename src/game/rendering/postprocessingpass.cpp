#include "postprocessingpass.h"

#include "framework/tools/log.h"
#include "game/config/gameconfiguration.h"
#include "game/shaders/postprocessing.h"

std::shared_ptr<sf::RenderTexture>
PostProcessingPass::selectLevelTarget(const std::shared_ptr<sf::RenderTexture>& frame_target, bool level_loaded)
{
   auto& post_processing = PostProcessing::getInstance();

   _level_scope_active = level_loaded && post_processing.isActive() && post_processing.getScope() == PostProcessing::Scope::Level &&
                         frame_target && createRenderTexture(frame_target->getSize());

   if (!_level_scope_active)
   {
      return frame_target;
   }

   // Level::draw blits with view_to_texture_scale, which only fills the target when that target
   // carries the level view. the frame target happens to have it left over from the previous
   // frame's overlays, a freshly created target does not, so it is set explicitly here rather than
   // relying on that leftover
#ifndef __EMSCRIPTEN__
   const auto& game_configuration = GameConfiguration::getInstance();
   const sf::View level_view{
      sf::FloatRect{{0.0f, 0.0f}, {static_cast<float>(game_configuration._view_width), static_cast<float>(game_configuration._view_height)}}
   };
   _render_texture->setView(level_view);
   frame_target->setView(level_view);
#endif

   _render_texture->clear();
   return _render_texture;
}

void PostProcessingPass::resolveLevelTarget(sf::RenderTexture& frame_target, float view_to_texture_scale)
{
   if (!_level_scope_active)
   {
      return;
   }

   _render_texture->display();

   const sf::Texture& texture = _render_texture->getTexture();
   const auto* shader = PostProcessing::getInstance().prepare(texture);

#ifdef __EMSCRIPTEN__
   sf::Sprite sprite;
   sprite.textureRect = sf::FloatRect{{0.f, 0.f}, {static_cast<float>(texture.getSize().x), static_cast<float>(texture.getSize().y)}};
   frame_target.draw(sprite, sf::RenderStates{.texture = &texture, .shader = shader});
#else
   // the frame target carries the level's view, so this blit has to use the same scale Level::draw
   // uses. without it the sprite covers several times the view, only a corner of it stays visible
   // and the effect sees texture coordinates spanning 0..1/scale, which makes anything resolution
   // dependent (the game boy grid) come out far too coarse
   auto sprite = sf::Sprite(texture);
   sprite.scale({view_to_texture_scale, view_to_texture_scale});
   frame_target.draw(sprite, shader);
#endif
}

const sf::Shader* PostProcessingPass::getFrameShader(const sf::Texture& frame_texture)
{
   auto& post_processing = PostProcessing::getInstance();

   // a level-scoped effect has already been resolved, and deliberately does nothing at all while
   // no level is running
   if (post_processing.getScope() != PostProcessing::Scope::All)
   {
      return nullptr;
   }

   return post_processing.prepare(frame_texture);
}

void PostProcessingPass::release()
{
   _render_texture.reset();
   _level_scope_active = false;
}

bool PostProcessingPass::createRenderTexture(const sf::Vector2u& size)
{
   if (_render_texture)
   {
      return true;
   }

#ifndef __EMSCRIPTEN__
   _render_texture = std::make_shared<sf::RenderTexture>(size);
#else
   _render_texture = std::make_shared<sf::RenderTexture>(std::move(*sf::RenderTexture::create(size)));
#endif

   Log::Info() << "created post processing render texture: " << size.x << " x " << size.y;
   return true;
}
