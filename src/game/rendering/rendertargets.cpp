#include "rendertargets.h"

// game
#include "framework/tools/log.h"
#include "game/config/gameconfiguration.h"

RenderTargetProfile RenderTargetProfile::full()
{
   return RenderTargetProfile{};
}

RenderTargetProfile RenderTargetProfile::reduced()
{
   // the image stays at full size: it is what the player looks at, and the art is not all on the
   // pixel grid. everything else here feeds the lighting maths rather than the eye
   return RenderTargetProfile{.image_scale = 1.0f, .lighting_scale = 0.5f, .normal_scale = 0.5f, .atmosphere_scale = 0.5f};
}

RenderTargetProfile RenderTargetProfile::fromName(const std::string& name)
{
   if (name == "reduced")
   {
      return reduced();
   }

   return full();
}

void RenderTargets::create(uint32_t video_mode_width, uint32_t video_mode_height, float view_width, float view_height)
{
#ifndef DECEPTUS_VRSFML
   // since stencil buffers are used, it is required to enable them explicitly
   sf::ContextSettings stencil_context_settings;
   stencil_context_settings.stencilBits = 8;
#endif

   // calculate texture size based on view dimensions. the scale has to be a whole number: the view
   // is pixel art, and rasterising it at a fractional multiple lands single art pixels across screen
   // pixel boundaries, which reads as a blurred edge rather than as a pixel
   const auto size_ratio = static_cast<float>(GameConfiguration::computeViewScale(
      static_cast<int32_t>(video_mode_width),
      static_cast<int32_t>(video_mode_height),
      static_cast<int32_t>(view_width),
      static_cast<int32_t>(view_height)
   ));
   view_to_texture_scale = 1.0f / size_ratio;

   const auto texture_width = static_cast<int32_t>(size_ratio * view_width);
   const auto texture_height = static_cast<int32_t>(size_ratio * view_height);

   profile = RenderTargetProfile::fromName(GameConfiguration::getInstance()._render_target_profile);

   // a group rendered smaller is stretched back over the image when it is sampled, so it has to
   // interpolate rather than pick the nearest texel - otherwise half size lighting reads as blocks
   const auto scaled_size = [texture_width, texture_height](float scale)
   {
      return sf::Vector2u{
         static_cast<uint32_t>(std::max(1.0f, static_cast<float>(texture_width) * scale)),
         static_cast<uint32_t>(std::max(1.0f, static_cast<float>(texture_height) * scale))
      };
   };

   const auto image_size = scaled_size(profile.image_scale);
   const auto lighting_size = scaled_size(profile.lighting_scale);
   const auto normal_size = scaled_size(profile.normal_scale);
   const auto atmosphere_size = scaled_size(profile.atmosphere_scale);

#ifdef DECEPTUS_VRSFML
   const auto texture_size = sf::Vector2u{static_cast<uint32_t>(texture_width), static_cast<uint32_t>(texture_height)};
   const sf::RenderTextureCreateSettings stencil_settings{.stencilBits = 8u};

   auto make_rt = [](sf::Vector2u size) -> std::shared_ptr<sf::RenderTexture>
   { return std::make_shared<sf::RenderTexture>(std::move(*sf::RenderTexture::create(size))); };
   auto make_rt_stencil = [&stencil_settings](sf::Vector2u size) -> std::shared_ptr<sf::RenderTexture>
   { return std::make_shared<sf::RenderTexture>(std::move(*sf::RenderTexture::create(size, stencil_settings))); };

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
#ifdef GLOW_ENABLED
   blur = make_rt_stencil(texture_size);
   blur_scaled = make_rt_stencil(sf::Vector2u{960u, 540u});
   blur_scaled->setSmooth(true);
#endif
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
#ifdef GLOW_ENABLED
      blur = std::make_shared<sf::RenderTexture>(texture_size, stencil_context_settings);
      blur_scaled = std::make_shared<sf::RenderTexture>(sf::Vector2u{960, 540}, stencil_context_settings);
      blur_scaled->setSmooth(true);
#endif
   }
   catch (const std::exception& e)
   {
      Log::Fatal() << "failed to create render textures: " << e.what();
   }
#endif

   // a group rendered smaller is stretched back over the image when it is sampled, so it has to
   // interpolate rather than pick the nearest texel
   for (const auto& scaled_target : {lighting, lighting2, normal, normal_tmp, atmosphere})
   {
      if (scaled_target)
      {
         scaled_target->setSmooth(true);
      }
   }

   _all_textures.clear();
   _all_textures.push_back(level);
   _all_textures.push_back(level_background);
   _all_textures.push_back(lighting);
   _all_textures.push_back(lighting2);
   _all_textures.push_back(normal);
   _all_textures.push_back(normal_tmp);
   _all_textures.push_back(deferred);
   _all_textures.push_back(atmosphere);
#ifdef GLOW_ENABLED
   _all_textures.push_back(blur);
   _all_textures.push_back(blur_scaled);
#endif

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
#ifdef GLOW_ENABLED
   blur.reset();
   blur_scaled.reset();
#endif
   _all_textures.clear();

   create(video_mode_width, video_mode_height, view_width, view_height);
}

const std::vector<std::shared_ptr<sf::RenderTexture>>& RenderTargets::getAll() const
{
   return _all_textures;
}
