#include "splash_animation.h"

#include "framework/easings/easings.h"
#include "framework/image/psd.h"
#include "framework/tools/assetsource.h"
#include "framework/tools/sfmlcompat.h"
#include "json/json.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#ifdef __SWITCH__
#include <switch.h>
#endif

namespace
{
using json = nlohmann::json;

enum class Easing
{
   Linear,
   EaseOutQuad,
   Smoothstep
};

[[nodiscard]] float readNumber(const json& value)
{
   if (!value.is_number() || !std::isfinite(value.get<float>()))
   {
      throw std::runtime_error("animation values must be finite numbers");
   }
   return value.get<float>();
}

[[nodiscard]] sf::Vector2f readVector(const json& value)
{
   if (!value.is_array() || value.size() != 2)
   {
      throw std::runtime_error("animation vectors must have two components");
   }
   return {readNumber(value.at(0)), readNumber(value.at(1))};
}

struct Transition
{
   float _start = 0.0f;
   float _duration = 0.0f;
   Easing _function = Easing::Linear;
   float _power = 1.0f;

   /// \brief evaluates the interval at an absolute time; zero-length intervals are instantaneous.
   [[nodiscard]] float evaluate(float time) const
   {
      if (_duration == 0.0f)
      {
         return time >= _start ? 1.0f : 0.0f;
      }
      auto ratio = std::clamp((time - _start) / _duration, 0.0f, 1.0f);
      if (_function == Easing::EaseOutQuad)
      {
         ratio = Easings::easeOutQuad(ratio);
      }
      else if (_function == Easing::Smoothstep)
      {
         ratio = ratio * ratio * (3.0f - 2.0f * ratio);
      }
      return ratio;
   }
};

[[nodiscard]] Transition readTransition(const json& value)
{
   Transition result;
   result._start = readNumber(value.value("start", json(0.0f)));
   result._duration = readNumber(value.at("duration"));
   result._power = readNumber(value.value("power", json(1.0f)));
   if (result._duration < 0.0f || result._power <= 0.0f)
   {
      throw std::runtime_error("animation duration must be nonnegative and power must be positive");
   }
   const auto name = value.value("easing", std::string("linear"));
   if (name == "ease_out_quad")
   {
      result._function = Easing::EaseOutQuad;
   }
   else if (name == "smoothstep")
   {
      result._function = Easing::Smoothstep;
   }
   else if (name != "linear")
   {
      throw std::runtime_error("unknown animation easing: " + name);
   }
   return result;
}

struct VectorCurve
{
   sf::Vector2f _from;
   sf::Vector2f _to;
   Transition _interval;

   /// \brief samples the transform without accumulating frame-dependent state.
   [[nodiscard]] sf::Vector2f evaluate(float time) const
   {
      const auto ratio = std::pow(_interval.evaluate(time), _interval._power);
      return _from + (_to - _from) * ratio;
   }
};

[[nodiscard]] VectorCurve readCurve(const json& value)
{
   if (value.is_array())
   {
      const auto constant = readVector(value);
      return {constant, constant, {}};
   }
   return {readVector(value.at("from")), readVector(value.at("to")), readTransition(value)};
}

void uploadSplashPixels(sf::Texture& texture, const PSD::Layer& layer)
{
   const auto& decoded_pixels = layer.getImage().getData();
   if (std::ranges::find(decoded_pixels, 0u) == decoded_pixels.end())
   {
      texture.update(reinterpret_cast<const uint8_t*>(decoded_pixels.data()));
      return;
   }

   // restore transparent edge colors before linear filtering the scaled splash artwork.
   const auto& red = layer.getChannel(0).data();
   const auto& green = layer.getChannel(1).data();
   const auto& blue = layer.getChannel(2).data();
   auto pixels = decoded_pixels;
   for (size_t index = 0; index < pixels.size(); ++index)
   {
      if (pixels[index] == 0)
      {
         pixels[index] =
            static_cast<uint32_t>(red[index]) | (static_cast<uint32_t>(green[index]) << 8) | (static_cast<uint32_t>(blue[index]) << 16);
      }
   }
   texture.update(reinterpret_cast<const uint8_t*>(pixels.data()));
}

[[nodiscard]] std::shared_ptr<sf::Texture> loadPsdTexture(const PSD::Layer& layer)
{
   const sf::Vector2u size{static_cast<uint32_t>(layer.getWidth()), static_cast<uint32_t>(layer.getHeight())};
#ifdef DECEPTUS_VRSFML
   auto decoded = sf::Texture::create(size);
   if (!decoded.hasValue())
   {
      throw std::runtime_error("failed to create texture for PSD layer: " + layer.getName());
   }
   auto texture = std::make_shared<sf::Texture>(std::move(*decoded));
#else
   auto texture = std::make_shared<sf::Texture>(size);
#endif
   uploadSplashPixels(*texture, layer);
   texture->setSmooth(true);
   return texture;
}

struct SpritePart
{
   sf::IntRect _rect;
   sf::Vector2f _offset;
};

struct SpriteLayer
{
   std::shared_ptr<sf::Texture> _texture;
   std::vector<SpritePart> _parts;
   sf::Vector2f _origin;
   VectorCurve _position;
   VectorCurve _scale;
   VectorCurve _spread;
   float _opacity = 1.0f;
   std::optional<Transition> _fade_in;
   std::optional<Transition> _fade_out;
};

[[nodiscard]] sf::IntRect readRect(const json& value, sf::Vector2u size)
{
   if (!value.is_array() || value.size() != 4)
   {
      throw std::runtime_error("animation rectangles must have four components");
   }
   const auto x = readNumber(value.at(0));
   const auto y = readNumber(value.at(1));
   const auto width = readNumber(value.at(2));
   const auto height = readNumber(value.at(3));
   if (x < 0 || y < 0 || width <= 0 || height <= 0 || x + width > size.x || y + height > size.y || std::floor(x) != x ||
       std::floor(y) != y || std::floor(width) != width || std::floor(height) != height)
   {
      throw std::runtime_error("animation rectangle is outside its texture or nonintegral");
   }
   return {{static_cast<int>(x), static_cast<int>(y)}, {static_cast<int>(width), static_cast<int>(height)}};
}

using TextureCache = std::map<std::string, std::shared_ptr<sf::Texture>>;

[[nodiscard]] sf::Color readClearColor(const json& config)
{
   if (config.count("clear_color") == 0)
   {
      return sf::Color::Black;
   }
   const auto& color = config.at("clear_color");
   if (!color.is_array() || color.size() != 3)
   {
      throw std::runtime_error("animation clear color must have three channels");
   }
   for (const auto& component : color)
   {
      const auto value = readNumber(component);
      if (value < 0 || value > 255 || std::floor(value) != value)
      {
         throw std::runtime_error("animation color channels must be integers in 0..255");
      }
   }
   return {color.at(0).get<uint8_t>(), color.at(1).get<uint8_t>(), color.at(2).get<uint8_t>()};
}

[[nodiscard]] std::vector<SplashAnimation::SoundCue> readSoundCues(const json& sounds, float duration)
{
   if (!sounds.is_array())
   {
      throw std::runtime_error("animation sounds must be an array");
   }
   std::vector<SplashAnimation::SoundCue> cues;
   for (const auto& sound : sounds)
   {
      auto sample = sound.at("sample").get<std::string>();
      const auto start = readNumber(sound.value("start", json(0.0f)));
      if (sample.empty() || start < 0.0f || start >= duration)
      {
         throw std::runtime_error("animation sound cues need a sample and a start within the animation");
      }
      cues.push_back({std::move(sample), start});
   }
   std::ranges::stable_sort(cues, {}, &SplashAnimation::SoundCue::_start);
   return cues;
}

[[nodiscard]] std::vector<SpritePart> readParts(const json& layer, sf::Vector2u texture_size)
{
   if (layer.count("parts") == 0)
   {
      return {{{{0, 0}, {static_cast<int>(texture_size.x), static_cast<int>(texture_size.y)}}, {0.0f, 0.0f}}};
   }
   if (!layer.at("parts").is_array())
   {
      throw std::runtime_error("animation parts must be an array");
   }
   std::vector<SpritePart> parts;
   for (const auto& part : layer.at("parts"))
   {
      parts.push_back({readRect(part.at("rect"), texture_size), readVector(part.value("offset", json::array({0, 0})))});
   }
   return parts;
}

[[nodiscard]] SpriteLayer readLayer(const json& item, const PSD& psd, TextureCache& textures)
{
   const auto name = item.at("psd_layer").get<std::string>();
   const auto source = psd.getLayer(name);
   if (source == psd.getLayers().end() || !source->isImageLayer())
   {
      throw std::runtime_error("missing drawable PSD layer: " + name);
   }
   auto& texture = textures[name];
   if (!texture)
   {
      texture = loadPsdTexture(*source);
   }

   SpriteLayer layer;
   layer._texture = texture;
   layer._parts = readParts(item, texture->getSize());
   layer._origin = readVector(item.value("origin", json::array({0, 0})));
   layer._position = readCurve(item.value("position", json::array({source->getLeft(), source->getTop()})));
   layer._scale = readCurve(item.value("scale", json::array({1, 1})));
   layer._spread = readCurve(item.value("spread", json::array({1, 1})));
   layer._opacity = readNumber(item.value("opacity", json(1.0f)));
   if (layer._opacity < 0 || layer._opacity > 1)
   {
      throw std::runtime_error("animation opacity must be in 0..1");
   }
   layer._opacity *= source->getOpacity() / 255.0f;
   if (item.count("fade_in") != 0)
   {
      layer._fade_in = readTransition(item.at("fade_in"));
   }
   if (item.count("fade_out") != 0)
   {
      layer._fade_out = readTransition(item.at("fade_out"));
   }
   return layer;
}

[[nodiscard]] sf::View createFittedView(sf::Vector2f canvas, sf::Vector2u target_size)
{
   const auto scale = std::min(target_size.x / canvas.x, target_size.y / canvas.y);
   const sf::FloatRect viewport{
      {(1.0f - canvas.x * scale / target_size.x) * 0.5f, (1.0f - canvas.y * scale / target_size.y) * 0.5f},
      {canvas.x * scale / target_size.x, canvas.y * scale / target_size.y}
   };
#ifdef DECEPTUS_VRSFML
   auto view = sf::View::fromRect(sf::FloatRect{{0.0f, 0.0f}, canvas});
   view.viewport = viewport;
#else
   sf::View view(sf::FloatRect{{0.0f, 0.0f}, canvas});
   view.setViewport(viewport);
#endif
   return view;
}

[[nodiscard]] uint8_t evaluateAlpha(const SpriteLayer& layer, float time)
{
   auto opacity = layer._opacity;
   if (layer._fade_in)
   {
      opacity *= std::pow(layer._fade_in->evaluate(time), layer._fade_in->_power);
   }
   if (layer._fade_out)
   {
      opacity *= std::pow(1.0f - layer._fade_out->evaluate(time), layer._fade_out->_power);
   }
   return static_cast<uint8_t>(std::clamp(std::round(255.0f * opacity), 0.0f, 255.0f));
}

void drawLayer(sf::RenderTarget& target, const SpriteLayer& layer, float time, const sf::RenderStates& states)
{
   const auto position = layer._position.evaluate(time);
   const auto scale = layer._scale.evaluate(time);
   const auto spread = layer._spread.evaluate(time);
   const sf::Color color{255, 255, 255, evaluateAlpha(layer, time)};
   for (const auto& part : layer._parts)
   {
      const sf::Vector2f part_position{
         position.x + part._offset.x * spread.x - layer._origin.x * part._rect.size.x * scale.x,
         position.y + part._offset.y * spread.y - layer._origin.y * part._rect.size.y * scale.y
      };
      sfcompat::drawTextureRegion(target, *layer._texture, part._rect, part_position, scale, color, states);
   }
}
}  // namespace

struct SplashAnimation::Implementation
{
   sf::Vector2f _canvas;
   float _duration = 0.0f;
   sf::Color _clear_color = sf::Color::Black;
   std::vector<SpriteLayer> _layers;
   std::vector<SoundCue> _sound_cues;
};

SplashAnimation::SplashAnimation(const std::string& config_path) : _implementation(std::make_unique<Implementation>())
{
   const auto contents = AssetSource::readFile(config_path);
   if (!contents)
   {
      throw std::runtime_error("missing animation configuration: " + config_path);
   }
   const auto config = json::parse(*contents);
   auto& data = *_implementation;
   data._canvas = readVector(config.at("canvas"));
   data._duration = readNumber(config.at("duration"));
   if (data._canvas.x <= 0 || data._canvas.y <= 0 || data._duration <= 0)
   {
      throw std::runtime_error("animation canvas and duration must be positive");
   }
   data._clear_color = readClearColor(config);
   if (const auto sounds = config.find("sounds"); sounds != config.end())
   {
      data._sound_cues = readSoundCues(*sounds, data._duration);
   }
   const auto& layers = config.at("layers");
   if (!layers.is_array())
   {
      throw std::runtime_error("animation layers must be an array");
   }
   const auto psd_path = config.at("psd").get<std::string>();
   PSD psd;
   psd.setColorFormat(PSD::ColorFormat::ABGR);
   if (!psd.load(psd_path))
   {
      throw std::runtime_error("missing or invalid animation PSD: " + psd_path);
   }
   TextureCache textures;
   for (const auto& layer : layers)
   {
      data._layers.push_back(readLayer(layer, psd, textures));
   }
}

SplashAnimation::~SplashAnimation() = default;

float SplashAnimation::getDuration() const
{
   return _implementation->_duration;
}

std::span<const SplashAnimation::SoundCue> SplashAnimation::getSoundCues() const
{
   return _implementation->_sound_cues;
}

void SplashAnimation::drawFrame(sf::RenderTarget& target, float time) const
{
   if (!std::isfinite(time))
   {
      throw std::runtime_error("animation time must be finite");
   }
   const auto size = target.getSize();
   if (size.x == 0 || size.y == 0)
   {
      return;
   }
   const auto& data = *_implementation;
   const auto view = createFittedView(data._canvas, size);
   sf::RenderStates states;
#ifdef DECEPTUS_VRSFML
   states.view = view;
#else
   const auto previous_view = target.getView();
   target.setView(view);
#endif
   target.clear(data._clear_color);
   for (const auto& layer : data._layers)
   {
      drawLayer(target, layer, time, states);
   }
#ifndef DECEPTUS_VRSFML
   target.setView(previous_view);
#endif
}

void SplashAnimation::play(sf::RenderWindow& window, const std::function<void(const std::string&)>& play_sound) const
{
   auto remaining_cues = play_sound ? getSoundCues() : std::span<const SoundCue>{};
   sf::Clock clock;
   for (auto time = 0.0f; time < getDuration(); time = clock.getElapsedTime().asSeconds())
   {
#ifdef __SWITCH__
      if (!appletMainLoop())
      {
         return;
      }
#endif
      while (const auto event = window.pollEvent())
      {
         if (event->is<sf::Event::Closed>())
         {
#ifndef DECEPTUS_VRSFML
            window.close();
#endif
            return;
         }
      }
      while (!remaining_cues.empty() && remaining_cues.front()._start <= time)
      {
         play_sound(remaining_cues.front()._sample);
         remaining_cues = remaining_cues.subspan(1);
      }
      drawFrame(window, time);
      window.display();
#ifdef __EMSCRIPTEN__
      // browser presentation and input require yielding even when native frame limiting is disabled.
      emscripten_sleep(0);
#endif
   }
   window.clear(_implementation->_clear_color);
   window.display();
}
