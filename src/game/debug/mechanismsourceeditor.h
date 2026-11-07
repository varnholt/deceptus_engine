#pragma once

#ifdef DECEPTUS_MECHANISM_INSPECTOR

#include <filesystem>
#include <span>
#include <string>
#include <vector>

struct MechanismSchema;

namespace MechanismSourceEditor
{
struct PropertyOverride
{
   std::string name;
   std::string value;
};

struct ObjectOverride
{
   std::string object_id;
   const MechanismSchema* schema{nullptr};
   std::vector<PropertyOverride> properties;
   std::string element_type{"object"};
};

bool writeObjects(
   const std::filesystem::path& path,
   const std::string& expected_source,
   std::span<const ObjectOverride> objects,
   std::string& message,
   std::string& written_source
);

}  // namespace MechanismSourceEditor

#endif
