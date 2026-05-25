#include "mechanismschemawriter.h"

#ifdef DEVELOPMENT_MODE

#include <filesystem>
#include <fstream>
#include <string>

#include "game/mechanisms/gamemechanismdeserializerregistry.h"
#include "json/json.hpp"

void writeMechanismSchemas()
{
   const auto& all_schemas = GameMechanismDeserializerRegistry::instance().schemas();
   auto json_array = nlohmann::json::array();
   for (const auto& schema : all_schemas)
   {
      auto json_properties = nlohmann::json::array();
      for (const auto& property_info : schema.properties)
      {
         json_properties.push_back(
            {{"name", std::string(property_info.name)},
             {"type", std::string(property_info.type)},
             {"default", std::string(property_info.default_value)},
             {"required", property_info.required}}
         );
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

#endif  // DEVELOPMENT_MODE
