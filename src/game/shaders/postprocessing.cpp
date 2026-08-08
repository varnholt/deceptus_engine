#include "postprocessing.h"

#include "framework/tools/log.h"
#include "game/config/gameconfiguration.h"
#include "game/mechanisms/postprocessingmechanism.h"

#include <algorithm>
#include <iterator>
#include <string_view>
#include <utility>

namespace
{

//! \brief the game boy screen matches the view in each direction, so the quantization snaps to the
//!        game's own pixel grid and only the palette reduction remains visible. must stay an integer
//!        divisor of the 640x360 view, otherwise the grid shimmers against the pixel grid
constexpr auto gameboy_grid_divisor = 1.0f;

//! \brief tone curve mapping scene luminance onto the 4 palette steps of the game boy effect. the
//!        game is lit far below the palette thresholds, so without it the whole frame collapses
//!        into the darkest color. the mid grey is the luminance that lands halfway up the palette,
//!        tuned against the catacombs so the player keeps its shading; raise it to darken
constexpr auto gameboy_black_point = 0.0f;
constexpr auto gameboy_mid_grey = 0.17f;

//! \brief horizontal channel separation of the rgb split effect, in game pixels
constexpr auto rgb_split_offset_px = 2.0f;

struct EffectName
{
   PostProcessing::Effect _effect;
   std::string_view _name;
};

constexpr EffectName effect_names[] = {
   {PostProcessing::Effect::None, "none"},
   {PostProcessing::Effect::GameBoy, "gameboy"},
   {PostProcessing::Effect::RgbSplit, "rgbsplit"},
   {PostProcessing::Effect::Glitch, "glitch"},
};

struct ScopeName
{
   PostProcessing::Scope _scope;
   std::string_view _name;
};

constexpr ScopeName scope_names[] = {
   {PostProcessing::Scope::All, "all"},
   {PostProcessing::Scope::Level, "level"},
};

//! \brief computes the size of one game pixel in uv space of the composited frame.
//!        the frame is rendered at an integer multiple of the view, so tying the effect
//!        parameters to the view keeps them stable across resolutions.
sf::Glsl::Vec2 getPixelSize()
{
   const auto& game_configuration = GameConfiguration::getInstance();
   return {1.0f / static_cast<float>(game_configuration._view_width), 1.0f / static_cast<float>(game_configuration._view_height)};
}

}  // namespace

PostProcessing& PostProcessing::getInstance()
{
   static PostProcessing __instance;
   return __instance;
}

void PostProcessing::initialize()
{
   if (_initialized)
   {
      return;
   }

   const std::vector<std::pair<Effect, std::string>> effect_sources = {
      {Effect::GameBoy, "data/shaders/gameboy.frag"},
      {Effect::RgbSplit, "data/shaders/rgb_split.frag"},
      {Effect::Glitch, "data/shaders/glitch.frag"},
   };

   _effect_shaders.reserve(effect_sources.size());

   for (const auto& [effect, fragment_path] : effect_sources)
   {
      EffectShader effect_shader;
      effect_shader._effect = effect;
      effect_shader._fragment_path = fragment_path;

      if (!effect_shader._shader.loadFromFragment(fragment_path))
      {
         Log::Error() << "error loading post processing shader: " << fragment_path;
         continue;
      }

      _effect_shaders.push_back(std::move(effect_shader));
   }

   _initialized = true;
}

void PostProcessing::update(const sf::Time& dt)
{
   _elapsed_s += dt.asSeconds();
}

void PostProcessing::setEffect(Effect effect)
{
   _effect = effect;
}

PostProcessing::Effect PostProcessing::getEffect() const
{
   return _effect;
}

void PostProcessing::setScope(Scope scope)
{
   _scope = scope;
}

PostProcessing::Scope PostProcessing::getScope() const
{
   if (_effect != Effect::None)
   {
      return _scope;
   }

   const auto level_effect = _level_effect.lock();
   return level_effect ? level_effect->getScope() : _scope;
}

void PostProcessing::setLevelEffect(const std::shared_ptr<PostProcessingMechanism>& mechanism)
{
   _level_effect = mechanism;
}

bool PostProcessing::isActive() const
{
   if (_effect != Effect::None)
   {
      const auto it = std::ranges::find_if(_effect_shaders, [this](const auto& candidate) { return candidate._effect == _effect; });
      return (it != _effect_shaders.end()) && it->_shader.isLoaded();
   }

   return !_level_effect.expired();
}

PostProcessing::EffectShader* PostProcessing::findEffectShader(Effect effect)
{
   const auto it = std::ranges::find_if(_effect_shaders, [effect](const auto& candidate) { return candidate._effect == effect; });
   return (it == _effect_shaders.end()) ? nullptr : &(*it);
}

const sf::Shader* PostProcessing::prepare(const sf::Texture& texture)
{
   if (_effect == Effect::None)
   {
      // no console override, so whatever the level configured through a mechanism applies
      const auto level_effect = _level_effect.lock();
      return level_effect ? level_effect->prepare(texture) : nullptr;
   }

   auto* effect_shader = findEffectShader(_effect);
   if (effect_shader == nullptr || !effect_shader->_shader.isLoaded())
   {
      return nullptr;
   }

   auto& shader = effect_shader->_shader;
   shader.setUniform("u_texture", texture);

   switch (_effect)
   {
      case Effect::GameBoy:
      {
         const auto& game_configuration = GameConfiguration::getInstance();
         shader.setUniform(
            "u_grid_size",
            sf::Glsl::Vec2{
               static_cast<float>(game_configuration._view_width) / gameboy_grid_divisor,
               static_cast<float>(game_configuration._view_height) / gameboy_grid_divisor
            }
         );
         shader.setUniform("u_black_point", gameboy_black_point);
         shader.setUniform("u_mid_grey", gameboy_mid_grey);
         break;
      }
      case Effect::RgbSplit:
      {
         shader.setUniform("u_pixel_size", getPixelSize());
         shader.setUniform("u_offset", rgb_split_offset_px);
         shader.setUniform("u_time", _elapsed_s);
         break;
      }
      case Effect::Glitch:
      {
         shader.setUniform("u_pixel_size", getPixelSize());
         shader.setUniform("u_time", _elapsed_s);
         break;
      }
      case Effect::None:
      {
         break;
      }
   }

   return &shader.native();
}

std::optional<PostProcessing::Effect> PostProcessing::effectFromName(const std::string& name)
{
   const auto it = std::ranges::find_if(effect_names, [&name](const auto& candidate) { return candidate._name == name; });
   return (it == std::end(effect_names)) ? std::nullopt : std::optional<Effect>{it->_effect};
}

std::vector<std::string> PostProcessing::getEffectNames()
{
   std::vector<std::string> names;
   names.reserve(std::size(effect_names));
   for (const auto& entry : effect_names)
   {
      names.emplace_back(entry._name);
   }
   return names;
}

std::optional<PostProcessing::Scope> PostProcessing::scopeFromName(const std::string& name)
{
   const auto it = std::ranges::find_if(scope_names, [&name](const auto& candidate) { return candidate._name == name; });
   return (it == std::end(scope_names)) ? std::nullopt : std::optional<Scope>{it->_scope};
}

std::vector<std::string> PostProcessing::getScopeNames()
{
   std::vector<std::string> names;
   names.reserve(std::size(scope_names));
   for (const auto& entry : scope_names)
   {
      names.emplace_back(entry._name);
   }
   return names;
}
