#pragma once

#include <SFML/Graphics.hpp>

#include "json/json.hpp"

#include <stdint.h>
#include <cstdint>
#include <filesystem>
#include <vector>

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

   void createFrames();

   AnimationSettings() = default;

   AnimationSettings(const AnimationSettings&);

   AnimationSettings(
      const std::array<int32_t, 2>& frameSize,
      const std::array<int32_t, 2>& frameOffset,
      const std::array<float, 2>& origin,
      const std::vector<sf::Time>& frameDurations,
      const std::filesystem::path& texturePath
   );
};

void from_json(const nlohmann::json& j, AnimationSettings& settings);
void to_json(nlohmann::json& j, const AnimationSettings& settings);
