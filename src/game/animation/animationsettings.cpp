#include "animationsettings.h"

void from_json(const nlohmann::json& j, AnimationSettings& settings)
{
   settings._sprite_count = j.at("sprite_count").get<int32_t>();
   settings._frame_size = j.at("frame_size").get<std::array<int32_t, 2>>();
   settings._frame_offset = j.at("frame_offset").get<std::array<int32_t, 2>>();
   settings._origin = j.at("origin").get<std::array<float, 2>>();
   settings._texture_path = j.at("texture").get<std::string>();

   const auto frame_durations = j.at("frame_durations").get<std::vector<int32_t>>();

   // if user just enters a single frame duration, just duplicate that for each sprite
   if (frame_durations.size() == 1)
   {
      for (auto i = 0; i < settings._sprite_count; i++)
      {
         settings._frame_durations.push_back(sf::milliseconds(frame_durations.at(0)));
      }
   }
   else
   {
      // otherwise have one frame duration for each frame
      for (const auto duration : frame_durations)
      {
         settings._frame_durations.push_back(sf::milliseconds(duration));
      }

      if (static_cast<int32_t>(frame_durations.size()) != settings._sprite_count)
      {
         settings._valid = false;
      }
   }

   for (auto i = 0; i < settings._sprite_count; i++)
   {
      settings._frames.emplace_back(
         settings._frame_offset[0] + (i * settings._frame_size[0]),
         settings._frame_offset[1],
         settings._frame_size[0],
         settings._frame_size[1]
      );
   }
}

AnimationSettings::AnimationSettings(
   const std::array<int32_t, 2>& frameSize,
   const std::array<int32_t, 2>& frameOffset,
   const std::array<float, 2>& origin,
   const std::vector<sf::Time>& frameDurations,
   const std::filesystem::path& texturePath
)
    : _frame_size(frameSize), _frame_offset(frameOffset), _origin(origin), _frame_durations(frameDurations), _texture_path(texturePath)
{
}

AnimationSettings::AnimationSettings(const AnimationSettings& other)
    : _frame_size(other._frame_size),
      _frame_offset(other._frame_offset),
      _origin(other._origin),
      _frame_durations(other._frame_durations),
      _texture_path(other._texture_path),
      _texture(other._texture),
      _normal_map(other._normal_map),
      _frames(other._frames),
      _sprite_count(other._sprite_count),
      _valid(other._valid)
{
}

void to_json(nlohmann::json& j, const AnimationSettings& settings)
{
   std::vector<int32_t> durations;
   std::transform(
      settings._frame_durations.begin(),
      settings._frame_durations.end(),
      std::back_inserter(durations),
      [](auto& duration) { return duration.asMilliseconds(); }
   );

   j = nlohmann::json{
      {"sprite_count", durations.size()},
      {"frame_size", settings._frame_size},
      {"frame_offset", settings._frame_offset},
      {"origin", settings._origin},
      {"texture", settings._texture_path.string()},
      {"frame_durations", durations}
   };
}
