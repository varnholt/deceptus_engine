#pragma once

#include <SFML/Graphics.hpp>

#include "json/json.hpp"

#include <stdint.h>
#include <cstdint>
#include <filesystem>
#include <vector>

/// \brief serializable animation definition loaded from json and converted to frame rectangles.
struct AnimationSettings
{
   std::array<int32_t, 2> _frame_size{};
   std::array<int32_t, 2> _frame_offset{};
   std::array<float, 2> _origin{};
   std::vector<sf::Time> _frame_durations;
   std::filesystem::path _texture_path;
   std::shared_ptr<sf::Texture> _texture;
   std::shared_ptr<sf::Texture> _normal_map;
   std::vector<sf::IntRect> _frames;
   bool _valid{true};

   /// \brief rebuilds frame rectangles from frame size, offset, and frame duration count.
   void createFrames();

   /// \brief constructs an empty settings object.
   AnimationSettings() = default;

   /// \brief copies all setting fields, textures, and generated frame rectangles.
   /// \param other settings instance to copy.
   AnimationSettings(const AnimationSettings&);

   /// \brief constructs settings with explicit geometry, timing, origin, and texture path values.
   /// \param frameSize size of one animation frame in pixels.
   /// \param frameOffset top-left pixel offset of the first frame inside the texture.
   /// \param origin local origin used when applying transformable origin.
   /// \param frameDurations playback duration for each sequential frame.
   /// \param texturePath texture path used to load sprite graphics.
   AnimationSettings(
      const std::array<int32_t, 2>& frameSize,
      const std::array<int32_t, 2>& frameOffset,
      const std::array<float, 2>& origin,
      const std::vector<sf::Time>& frameDurations,
      const std::filesystem::path& texturePath
   );
};

/// \brief deserializes one animation settings entry from json into an AnimationSettings instance.
/// \param j json object containing frame geometry, durations, origin, and texture path.
/// \param settings destination settings object to populate.
void from_json(const nlohmann::json& j, AnimationSettings& settings);
/// \brief serializes an AnimationSettings instance into the json schema used by the editor.
/// \param j destination json object that receives serialized fields.
/// \param settings settings instance to serialize.
void to_json(nlohmann::json& j, const AnimationSettings& settings);
