#include "rendertargets.h"

// game
#include "framework/tools/log.h"
#include "game/config/gameconfiguration.h"

void RenderTargets::create(uint32_t video_mode_width, uint32_t video_mode_height, float view_width, float view_height)
{
#ifndef __EMSCRIPTEN__
   // since stencil buffers are used, it is required to enable them explicitly
   sf::ContextSettings stencil_context_settings;
   stencil_context_settings.stencilBits = 8;
#endif

   // calculate texture size based on view dimensions
   const auto ratio_width = video_mode_width / view_width;
   const auto ratio_height = video_mode_height / view_height;
   const auto size_ratio = std::min(ratio_width, ratio_height);
   view_to_texture_scale = 1.0f / size_ratio;

   const auto texture_width = static_cast<int32_t>(size_ratio * view_width);
   const auto texture_height = static_cast<int32_t>(size_ratio * view_height);

#ifdef __EMSCRIPTEN__
   const auto texture_size = sf::Vector2u{static_cast<uint32_t>(texture_width), static_cast<uint32_t>(texture_height)};
   const sf::RenderTextureCreateSettings stencil_settings{.stencilBits = 8u};

   auto make_rt = [](sf::Vector2u size) -> std::shared_ptr<sf::RenderTexture>
   {
      return std::make_shared<sf::RenderTexture>(std::move(*sf::RenderTexture::create(size)));
   };
   auto make_rt_stencil = [&stencil_settings](sf::Vector2u size) -> std::shared_ptr<sf::RenderTexture>
   {
      return std::make_shared<sf::RenderTexture>(std::move(*sf::RenderTexture::create(size, stencil_settings)));
   };

   level_background = make_rt(texture_size);
   level = make_rt_stencil(texture_size);
   lighting = make_rt_stencil(texture_size);
   lighting2 = make_rt_stencil(texture_size);

   // explicitly clear lighting textures to black on creation
   lighting->clear(sf::Color::Black);
   lighting->display();
   lighting2->clear(sf::Color::Black);
   lighting2->display();

   normal = make_rt(texture_size);
   normal_tmp = make_rt(texture_size);
   deferred = make_rt(texture_size);
   atmosphere = make_rt(texture_size);
   blur = make_rt_stencil(texture_size);
   blur_scaled = make_rt_stencil(sf::Vector2u{960u, 540u});
   blur_scaled->setSmooth(true);
#else
   try
   {
      const auto texture_size = sf::Vector2u{static_cast<uint32_t>(texture_width), static_cast<uint32_t>(texture_height)};
      level_background = std::make_shared<sf::RenderTexture>(texture_size);
      level = std::make_shared<sf::RenderTexture>(texture_size, stencil_context_settings);
      lighting = std::make_shared<sf::RenderTexture>(texture_size, stencil_context_settings);
      lighting2 = std::make_shared<sf::RenderTexture>(texture_size, stencil_context_settings);

      // explicitly clear lighting textures to black on creation
      lighting->clear(sf::Color::Black);
      lighting->display();
      lighting2->clear(sf::Color::Black);
      lighting2->display();

      normal = std::make_shared<sf::RenderTexture>(texture_size);
      normal_tmp = std::make_shared<sf::RenderTexture>(texture_size);
      deferred = std::make_shared<sf::RenderTexture>(texture_size);
      atmosphere = std::make_shared<sf::RenderTexture>(texture_size);
      blur = std::make_shared<sf::RenderTexture>(texture_size, stencil_context_settings);
      blur_scaled = std::make_shared<sf::RenderTexture>(sf::Vector2u{960, 540}, stencil_context_settings);
      blur_scaled->setSmooth(true);
   }
   catch (const std::exception& e)
   {
      Log::Fatal() << "failed to create render textures: " << e.what();
   }
#endif

   _all_textures.clear();
   _all_textures.push_back(level);
   _all_textures.push_back(level_background);
   _all_textures.push_back(lighting);
   _all_textures.push_back(lighting2);
   _all_textures.push_back(normal);
   _all_textures.push_back(normal_tmp);
   _all_textures.push_back(deferred);
   _all_textures.push_back(atmosphere);
   _all_textures.push_back(blur);
   _all_textures.push_back(blur_scaled);

   // for (const auto& texture : _all_textures)
   // {
   //    Log::Info() << "created render texture: " << texture->getSize().x << " x " << texture->getSize().y;
   // }
}

void RenderTargets::recreateOnResize(uint32_t video_mode_width, uint32_t video_mode_height, float view_width, float view_height)
{
   // reset all textures
   level_background.reset();
   level.reset();
   lighting.reset();
   lighting2.reset();
   normal.reset();
   normal_tmp.reset();
   deferred.reset();
   atmosphere.reset();
   blur.reset();
   blur_scaled.reset();
   _all_textures.clear();

   create(video_mode_width, video_mode_height, view_width, view_height);
}

const std::vector<std::shared_ptr<sf::RenderTexture>>& RenderTargets::getAll() const
{
   return _all_textures;
}
