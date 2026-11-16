#include "framework/image/splash_animation.h"
#include "json/json.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace
{
using json = nlohmann::json;

/// \brief reports a failed behavioral expectation in release builds as well as debug builds.
void require(bool condition, const char* message)
{
   if (!condition)
   {
      throw std::runtime_error(message);
   }
}

/// \brief writes one isolated configuration fixture and closes it before loading.
void writeConfig(const std::filesystem::path& path, const json& config)
{
   std::ofstream file(path);
   file << config.dump(2);
}

/// \brief samples a rendered frame from a caller-owned render target.
[[nodiscard]] sf::Image render(const SplashAnimation& animation, sf::RenderTexture& target, float time)
{
   animation.drawFrame(target, time);
   target.display();
   return target.getTexture().copyToImage();
}

/// \brief verifies that invalid input is rejected before playback begins.
void expectInvalid(const std::filesystem::path& path, const json& config)
{
   writeConfig(path, config);
   bool rejected = false;
   try
   {
      const SplashAnimation animation(path.string());
   }
   catch (const std::exception&)
   {
      rejected = true;
   }
   require(rejected, "invalid animation was accepted");
}
}  // namespace

/// \brief exercises arbitrary artwork, timing, view restoration, resizing and configuration failures.
int main(int argc, char** argv)
{
   try
   {
      require(argc == 2, "pass a writable fixture directory");
      const auto directory = std::filesystem::absolute(argv[1]);
      std::filesystem::create_directories(directory);
      const auto texture_path = directory / "parts.png";
      const auto config_path = directory / "animation.json";
      sf::Image texture({4, 2}, sf::Color::Red);
      for (auto y = 0u; y < 2; ++y)
      {
         for (auto x = 2u; x < 4; ++x)
         {
            texture.setPixel({x, y}, sf::Color::Green);
         }
      }
      require(texture.saveToFile(texture_path), "could not write fixture texture");
      const json config = {
         {"canvas", {100, 50}},
         {"duration", 2},
         {"layers",
          {{{"texture", texture_path.generic_string()},
            {"position", {{"from", {10, 10}}, {"to", {50, 10}}, {"start", 0}, {"duration", 1}, {"easing", "linear"}}},
            {"scale", {10, 10}},
            {"fade_in", {{"start", 0}, {"duration", 1}}},
            {"fade_out", {{"start", 1}, {"duration", 1}}},
            {"parts", {{{"rect", {0, 0, 2, 2}}}, {{"rect", {2, 0, 2, 2}}, {"offset", {30, 0}}}}}}}}
      };
      writeConfig(config_path, config);
      const SplashAnimation animation(config_path.string());
      require(animation.getDuration() == 2.0f, "configured duration was ignored");
      sf::RenderTexture target({100, 50});
      const sf::View previous_view(sf::FloatRect{{7, 9}, {20, 30}});
      target.setView(previous_view);
      const auto start = render(animation, target, 0.0f);
      require(start.getPixel({20, 20}) == sf::Color::Black, "fade did not start at black");
      const auto middle = render(animation, target, 0.5f);
      const auto red = middle.getPixel({40, 20});
      require(red.r >= 127 && red.r <= 128 && red.g == 0, "position interpolation or fade-in is incorrect");
      const auto held = render(animation, target, 1.0f);
      require(held.getPixel({60, 20}) == sf::Color::Red, "first sprite region is incorrect");
      require(held.getPixel({90, 20}) == sf::Color::Green, "second sprite region or offset is incorrect");
      const auto fading = render(animation, target, 1.5f);
      require(fading.getPixel({60, 20}).r >= 127 && fading.getPixel({60, 20}).r <= 128, "fade-out is incorrect");
      require(render(animation, target, 2.0f).getPixel({60, 20}) == sf::Color::Black, "fade did not end at black");
      require(render(animation, target, 1.0f).getPixel({60, 20}) == sf::Color::Red, "random access depends on previous frame time");
      require(
         target.getView().getCenter() == previous_view.getCenter() && target.getView().getSize() == previous_view.getSize(),
         "drawing changed the caller's view"
      );

      sf::RenderTexture tall({100, 100});
      const auto tall_frame = render(animation, tall, 1.0f);
      require(
         tall_frame.getPixel({60, 20}) == sf::Color::Black && tall_frame.getPixel({60, 45}) == sf::Color::Red &&
            tall_frame.getPixel({60, 80}) == sf::Color::Black,
         "tall target does not preserve the composition"
      );
      sf::RenderTexture wide({200, 50});
      const auto wide_frame = render(animation, wide, 1.0f);
      require(
         wide_frame.getPixel({10, 20}) == sf::Color::Black && wide_frame.getPixel({110, 20}) == sf::Color::Red &&
            wide_frame.getPixel({180, 20}) == sf::Color::Black,
         "wide target does not preserve the composition"
      );

      auto other = config;
      other["layers"][0]["position"] = {0, 0};
      other["layers"][0]["fade_in"]["duration"] = 0;
      other["layers"][0].erase("parts");
      writeConfig(config_path, other);
      const SplashAnimation independent(config_path.string());
      require(
         render(independent, target, 0.0f).getPixel({5, 5}) == sf::Color::Red, "whole-texture defaults or instant fades are incorrect"
      );
      require(render(animation, target, 1.0f).getPixel({60, 20}) == sf::Color::Red, "instances share mutable configuration");

      auto invalid = config;
      invalid["duration"] = -1;
      expectInvalid(config_path, invalid);
      invalid = config;
      invalid["canvas"] = {0, 50};
      expectInvalid(config_path, invalid);
      invalid = config;
      invalid["layers"][0]["position"]["easing"] = "unknown";
      expectInvalid(config_path, invalid);
      invalid = config;
      invalid["layers"][0]["parts"][0]["rect"] = {0, 0, 100, 2};
      expectInvalid(config_path, invalid);
      invalid = config;
      invalid["layers"][0]["texture"] = (directory / "missing.png").generic_string();
      expectInvalid(config_path, invalid);
      invalid = config;
      invalid["layers"][0]["fade_out"]["duration"] = -1;
      expectInvalid(config_path, invalid);
      std::cout << "splash animation behavior checks passed\n";
      return 0;
   }
   catch (const std::exception& error)
   {
      std::cerr << error.what() << '\n';
      return 1;
   }
}
