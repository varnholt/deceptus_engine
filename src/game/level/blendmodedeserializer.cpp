#include "blendmodedeserializer.h"

#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"

std::optional<sf::BlendMode> BlendModeDeserializer::readBlendMode(const std::map<std::string, std::shared_ptr<TmxProperty>>& map)
{
   auto blend_mode_it = map.find("blend_mode");
   if (blend_mode_it != map.end())
   {
      std::optional<sf::BlendMode> blend_mode;
      const auto blend_mode_name = blend_mode_it->second->_value_string.value();
      if (blend_mode_name == "add")
      {
         blend_mode = sf::BlendAdd;
      }
      else if (blend_mode_name == "alpha")
      {
         blend_mode = sf::BlendAlpha;
      }
      else if (blend_mode_name == "multiply")
      {
         blend_mode = sf::BlendMultiply;
      }
      else if (blend_mode_name == "min")
      {
         blend_mode = sf::BlendMin;
      }
      else if (blend_mode_name == "max")
      {
         blend_mode = sf::BlendMax;
      }
      else if (blend_mode_name == "none")
      {
         blend_mode = sf::BlendNone;
      }

      return blend_mode;
   }

   auto readEquation = [](const auto& equation_name) -> sf::BlendMode::Equation
   {
      sf::BlendMode::Equation equation{sf::BlendMode::Equation::Add};

      static std::map<std::string, sf::BlendMode::Equation> equation_map{
         {"add", sf::BlendMode::Equation::Add},
         {"subtract", sf::BlendMode::Equation::Subtract},
         {"reverse_subtract", sf::BlendMode::Equation::ReverseSubtract},
         {"min", sf::BlendMode::Equation::Min},
         {"max", sf::BlendMode::Equation::Max},
      };

      const auto equation_it = equation_map.find(equation_name);
      if (equation_it != equation_map.end())
      {
         equation = equation_it->second;
      }

      return equation;
   };

   auto readFactor = [](const auto& factor_name) -> sf::BlendMode::Factor
   {
      sf::BlendMode::Factor factor{sf::BlendMode::Factor::Zero};

      static std::map<std::string, sf::BlendMode::Factor> factor_map{
         {"zero", sf::BlendMode::Factor::Zero},
         {"one", sf::BlendMode::Factor::One},
         {"src_color", sf::BlendMode::Factor::SrcColor},
         {"one_minus_src_color", sf::BlendMode::Factor::OneMinusSrcColor},
         {"dst_color", sf::BlendMode::Factor::DstColor},
         {"one_minus_dst_color", sf::BlendMode::Factor::OneMinusDstColor},
         {"src_alpha", sf::BlendMode::Factor::SrcAlpha},
         {"one_minus_src_alpha", sf::BlendMode::Factor::OneMinusSrcAlpha},
         {"dst_alpha", sf::BlendMode::Factor::DstAlpha},
         {"one_minus_dst_alpha", sf::BlendMode::Factor::OneMinusDstAlpha},
      };

      const auto factor_it = factor_map.find(factor_name);
      if (factor_it != factor_map.end())
      {
         factor = factor_it->second;
      }

      return factor;
   };

   auto source_factor_it = map.find("source_factor");
   const auto destination_factor_it = map.find("destination_factor");
   const auto blend_equation_it = map.find("blend_equation");
   if (source_factor_it != map.end() && destination_factor_it != map.end() && blend_equation_it != map.end())
   {
      const auto source_factor_name = source_factor_it->second->_value_string.value();
      const auto source_factor = readFactor(source_factor_name);

      const auto destination_factor_name = destination_factor_it->second->_value_string.value();
      const auto destination_factor = readFactor(destination_factor_name);

      const auto blend_equation_name = blend_equation_it->second->_value_string.value();
      const auto blend_equation = readEquation(blend_equation_name);

      return sf::BlendMode{source_factor, destination_factor, blend_equation};
   }

   const auto color_source_factor_it = map.find("color_source_factor");
   const auto color_destination_factor_it = map.find("color_destination_factor");
   const auto color_blend_equation_it = map.find("color_blend_equation");
   const auto alpha_source_factor_it = map.find("alpha_source_factor");
   const auto alpha_destination_factor_it = map.find("alpha_destination_factor");
   const auto alpha_blend_equation_it = map.find("alpha_blend_equation");

   // clang-format off
   if (
         color_source_factor_it      != map.end()
      && color_destination_factor_it != map.end()
      && color_blend_equation_it     != map.end()
      && alpha_source_factor_it      != map.end()
      && alpha_destination_factor_it != map.end()
      && alpha_blend_equation_it     != map.end()
   )
   {
      // clang-format on
      const auto color_source_factor_name = color_source_factor_it->second->_value_string.value();
      const auto color_source_factor = readFactor(color_source_factor_name);
      const auto color_destination_factor_name = color_destination_factor_it->second->_value_string.value();
      const auto color_destination_factor = readFactor(color_destination_factor_name);
      const auto color_blend_equation_name = color_blend_equation_it->second->_value_string.value();
      const auto color_blend_equation = readEquation(color_blend_equation_name);

      const auto alpha_source_factor_name = alpha_source_factor_it->second->_value_string.value();
      const auto alpha_source_factor = readFactor(alpha_source_factor_name);
      const auto alpha_destination_factor_name = alpha_destination_factor_it->second->_value_string.value();
      const auto alpha_destination_factor = readFactor(alpha_destination_factor_name);
      const auto alpha_blend_equation_name = alpha_blend_equation_it->second->_value_string.value();
      const auto alpha_blend_equation = readEquation(alpha_blend_equation_name);

      return sf::BlendMode{
         color_source_factor,
         color_destination_factor,
         color_blend_equation,
         alpha_source_factor,
         alpha_destination_factor,
         alpha_blend_equation
      };
   }

   return std::nullopt;
}
