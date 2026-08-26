#include "mechanismschemawriter.h"

#ifdef MECHANISM_SCHEMA_WRITER_ENABLED

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include "game/mechanisms/gamemechanismdeserializerregistry.h"
#include "json/json.hpp"

namespace
{

//! \brief tiled's default color for a custom class
constexpr std::string_view tiled_class_color{"#ffa0a0a4"};

//! \brief path of the tiled project file that carries the generated custom types
constexpr std::string_view tiled_project_path{"deceptus.tiled-project"};

/// \brief converts a pascal case mechanism type name to the snake case template file name.
/// \param type_name mechanism type name such as "OnOffBlock".
/// \return snake case file stem such as "on_off_block".
std::string toSnakeCase(std::string_view type_name)
{
   std::string result;
   for (auto index = 0u; index < type_name.size(); index++)
   {
      const auto character = type_name[index];
      if (std::isupper(static_cast<unsigned char>(character)) && index > 0)
      {
         result.push_back('_');
      }
      result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));
   }
   return result;
}

/// \brief converts a snake case property name to pascal case so it can be appended to an enum type name.
/// \param property_name property name such as "blend_mode".
/// \return pascal case name such as "BlendMode".
std::string toPascalCase(std::string_view property_name)
{
   std::string result;
   auto capitalize_next = true;
   for (const auto character : property_name)
   {
      if (character == '_')
      {
         capitalize_next = true;
         continue;
      }
      result.push_back(capitalize_next ? static_cast<char>(std::toupper(static_cast<unsigned char>(character))) : character);
      capitalize_next = false;
   }
   return result;
}

/// \brief builds the tiled enum type name for a property that has a fixed set of accepted values.
/// \param type_name mechanism type name.
/// \param property_name property name.
/// \return enum type name such as "SmokeBlendMode".
std::string toEnumTypeName(std::string_view type_name, std::string_view property_name)
{
   return std::string{type_name} + toPascalCase(property_name);
}

/// \brief formats a float the way tiled does, so that regenerating a file does not churn its contents.
/// \param value value to format.
/// \return shortest representation using six significant digits.
std::string toTiledFloat(float value)
{
   std::array<char, 32> buffer{};
   std::snprintf(buffer.data(), buffer.size(), "%g", static_cast<double>(value));
   return std::string{buffer.data()};
}

/// \brief returns the value a newly inserted object should start with.
/// \param property_info property to read.
/// \return the property's template value when it has one, its engine default otherwise.
PropertyValue effectiveValue(const PropertyInfo& property_info)
{
   return property_info.template_value.value_or(property_info.default_value);
}

/// \brief renders a property value as the string that goes into a tmx value attribute.
/// \param value_variant value to render.
/// \return value as text.
std::string toValueString(const PropertyValue& value_variant)
{
   return std::visit(
      [](const auto& value) -> std::string
      {
         using ValueType = std::decay_t<decltype(value)>;
         if constexpr (std::is_same_v<ValueType, std::string_view>)
         {
            return std::string{value};
         }
         else if constexpr (std::is_same_v<ValueType, bool>)
         {
            return value ? "true" : "false";
         }
         else if constexpr (std::is_same_v<ValueType, float>)
         {
            return toTiledFloat(value);
         }
         else
         {
            return std::to_string(value);
         }
      },
      value_variant
   );
}

/// \brief renders a property value as the json value that goes into a tiled class member.
/// \param value_variant value to render.
/// \return value as json.
nlohmann::json toValueJson(const PropertyValue& value_variant)
{
   return std::visit(
      [](const auto& value) -> nlohmann::json
      {
         if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::string_view>)
         {
            return std::string{value};
         }
         else
         {
            return value;
         }
      },
      value_variant
   );
}

/// \brief escapes the characters that must not appear literally inside an xml attribute.
/// \param value text to escape.
/// \return escaped text.
std::string escapeXmlAttribute(std::string_view value)
{
   std::string result;
   for (const auto character : value)
   {
      switch (character)
      {
         case '&':
            result += "&amp;";
            break;
         case '<':
            result += "&lt;";
            break;
         case '>':
            result += "&gt;";
            break;
         case '"':
            result += "&quot;";
            break;
         default:
            result.push_back(character);
            break;
      }
   }
   return result;
}

/// \brief returns the mechanism's properties sorted by name, matching the order tiled writes them in.
/// \param schema schema to read the properties from.
/// \return pointers to the schema's properties, sorted by name.
std::vector<const PropertyInfo*> sortedProperties(const MechanismSchema& schema)
{
   std::vector<const PropertyInfo*> properties;
   properties.reserve(schema.properties.size());
   for (const auto& property_info : schema.properties)
   {
      properties.push_back(&property_info);
   }
   std::ranges::sort(properties, [](const auto* left, const auto* right) { return left->name < right->name; });
   return properties;
}

/// \brief writes data/schemas/mechanisms.json, the machine readable dump of all registered schemas.
/// \param all_schemas schemas to write.
void writeSchemaJson(const std::vector<MechanismSchema>& all_schemas)
{
   auto json_array = nlohmann::json::array();
   for (const auto& schema : all_schemas)
   {
      auto json_properties = nlohmann::json::array();
      for (const auto& property_info : schema.properties)
      {
         auto json_property = nlohmann::json{
            {"name", std::string(property_info.name)},
            {"type", std::string(property_info.type)},
            {"default", toValueJson(property_info.default_value)},
            {"required", property_info.required}
         };

         if (property_info.template_value.has_value())
         {
            json_property["template_default"] = toValueJson(property_info.template_value.value());
         }

         if (!property_info.allowed_values.empty())
         {
            auto json_allowed_values = nlohmann::json::array();
            for (const auto& allowed_value : property_info.allowed_values)
            {
               json_allowed_values.push_back(std::string(allowed_value));
            }
            json_property["allowed_values"] = json_allowed_values;
         }

         json_properties.push_back(json_property);
      }

      json_array.push_back(
         {{"type", std::string(schema.type_name)},
          {"layer", std::string(schema.layer_name)},
          {"default_width", schema.default_width},
          {"default_height", schema.default_height},
          {"properties", json_properties}}
      );
   }

   std::filesystem::create_directories("data/schemas");
   std::ofstream schemas_file("data/schemas/mechanisms.json");
   schemas_file << json_array.dump(3);
}

/// \brief writes one tiled template per mechanism into data/templates.
/// \param all_schemas schemas to write templates for.
void writeTemplates(const std::vector<MechanismSchema>& all_schemas)
{
   std::filesystem::create_directories("data/templates");

   for (const auto& schema : all_schemas)
   {
      std::ostringstream stream;
      stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
      stream << "<template>\n";
      stream << " <object type=\"" << escapeXmlAttribute(schema.type_name) << "\" width=\"" << schema.default_width << "\" height=\""
             << schema.default_height << "\">\n";

      if (!schema.properties.empty())
      {
         stream << "  <properties>\n";
         for (const auto* property_info : sortedProperties(schema))
         {
            stream << "   <property name=\"" << escapeXmlAttribute(property_info->name) << "\"";

            // tiled omits the type attribute for strings, which is also the tmx parser's fallback
            if (property_info->type != "string")
            {
               stream << " type=\"" << escapeXmlAttribute(property_info->type) << "\"";
            }

            if (!property_info->allowed_values.empty())
            {
               stream << " propertytype=\"" << escapeXmlAttribute(toEnumTypeName(schema.type_name, property_info->name)) << "\"";
            }

            stream << " value=\"" << escapeXmlAttribute(toValueString(effectiveValue(*property_info))) << "\"/>\n";
         }
         stream << "  </properties>\n";
      }

      if (!schema.default_polyline.empty())
      {
         stream << "  <polyline points=\"" << escapeXmlAttribute(schema.default_polyline) << "\"/>\n";
      }

      stream << " </object>\n";
      stream << "</template>\n";

      std::ofstream template_file("data/templates/" + toSnakeCase(schema.type_name) + ".tx");
      template_file << stream.str();
   }
}

/// \brief builds the tiled custom types for all mechanisms: one class per mechanism plus one enum per constrained property.
/// \param all_schemas schemas to derive the custom types from.
/// \return custom types keyed by name so that they end up sorted and can be merged with hand made ones.
std::map<std::string, nlohmann::json> buildPropertyTypes(const std::vector<MechanismSchema>& all_schemas)
{
   std::map<std::string, nlohmann::json> property_types;

   for (const auto& schema : all_schemas)
   {
      auto json_members = nlohmann::json::array();

      for (const auto* property_info : sortedProperties(schema))
      {
         auto json_member = nlohmann::json{
            {"name", std::string(property_info->name)},
            {"type", std::string(property_info->type)},
            {"value", toValueJson(effectiveValue(*property_info))}
         };

         if (!property_info->allowed_values.empty())
         {
            const auto enum_type_name = toEnumTypeName(schema.type_name, property_info->name);
            json_member["propertyType"] = enum_type_name;

            auto json_values = nlohmann::json::array();
            for (const auto& allowed_value : property_info->allowed_values)
            {
               json_values.push_back(std::string(allowed_value));
            }

            property_types[enum_type_name] = nlohmann::json{
               {"name", enum_type_name}, {"type", "enum"}, {"storageType", "string"}, {"values", json_values}, {"valuesAsFlags", false}
            };
         }

         json_members.push_back(json_member);
      }

      property_types[std::string(schema.type_name)] = nlohmann::json{
         {"name", std::string(schema.type_name)},
         {"type", "class"},
         {"color", std::string(tiled_class_color)},
         {"drawFill", true},
         {"members", json_members},
         {"useAs", nlohmann::json::array({"property", "object"})}
      };
   }

   return property_types;
}

/// \brief writes the tiled project file, replacing the generated custom types and keeping everything else untouched.
/// \param all_schemas schemas to derive the custom types from.
void writeTiledProject(const std::vector<MechanismSchema>& all_schemas)
{
   auto project = nlohmann::json{
      {"automappingRulesFile", ""},
      {"commands", nlohmann::json::array()},
      {"extensionsPath", "extensions"},
      {"folders", nlohmann::json::array({"data"})},
      {"properties", nlohmann::json::array()},
      {"propertyTypes", nlohmann::json::array()}
   };

   auto property_types = buildPropertyTypes(all_schemas);

   // keep custom types that were added by hand in tiled and that no mechanism owns
   std::ifstream existing_project_file{std::string{tiled_project_path}};
   if (existing_project_file.is_open())
   {
      const auto existing_project = nlohmann::json::parse(existing_project_file, nullptr, false);
      if (!existing_project.is_discarded() && existing_project.is_object())
      {
         for (const auto& [key, value] : existing_project.items())
         {
            if (key != "propertyTypes")
            {
               project[key] = value;
            }
         }

         for (const auto& existing_property_type : existing_project.value("propertyTypes", nlohmann::json::array()))
         {
            const auto name = existing_property_type.value("name", std::string{});
            if (!name.empty() && !property_types.contains(name))
            {
               property_types[name] = existing_property_type;
            }
         }
      }
   }
   existing_project_file.close();

   auto next_id = 1;
   auto json_property_types = nlohmann::json::array();
   for (auto& [name, property_type] : property_types)
   {
      property_type["id"] = next_id++;
      json_property_types.push_back(property_type);
   }
   project["propertyTypes"] = json_property_types;

   std::ofstream project_file{std::string{tiled_project_path}};
   project_file << project.dump(4) << "\n";
}

}  // namespace

void writeMechanismSchemas()
{
   const auto& all_schemas = GameMechanismDeserializerRegistry::instance().schemas();

   writeSchemaJson(all_schemas);
   writeTemplates(all_schemas);
   writeTiledProject(all_schemas);
}

#endif  // MECHANISM_SCHEMA_WRITER_ENABLED
