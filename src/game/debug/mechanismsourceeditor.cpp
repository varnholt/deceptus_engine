#include "mechanismsourceeditor.h"

#ifdef DECEPTUS_MECHANISM_INSPECTOR

#include "game/debug/mechanism_property_bindings.h"
#include "game/mechanisms/gamemechanismschema.h"
#include "tinyxml2/tinyxml2.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <ranges>
#include <regex>
#include <set>

namespace
{
tinyxml2::XMLElement* findObject(tinyxml2::XMLElement* parent, const std::string& id, const std::string& element_type)
{
   for (auto* child = parent->FirstChildElement(); child; child = child->NextSiblingElement())
   {
      const auto type = std::string_view{child->Name()};
      if (type == element_type && type == "layer")
      {
         if (const auto* layer_id = child->Attribute("id"); layer_id && id == layer_id)
            return child;
      }
      if (type == "objectgroup" && element_type == "object")
      {
         for (auto* object = child->FirstChildElement("object"); object; object = object->NextSiblingElement("object"))
         {
            if (const auto* object_id = object->Attribute("id"); object_id && id == object_id)
            {
               return object;
            }
         }
      }
      else if (type == "group")
      {
         if (auto* object = findObject(child, id, element_type))
         {
            return object;
         }
      }
   }
   return nullptr;
}

bool writeFile(const std::filesystem::path& path, const std::string& contents)
{
   std::ofstream file(path, std::ios::binary | std::ios::trunc);
   file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
   file.close();
   return !file.fail();
}
}  // namespace

bool MechanismSourceEditor::writeObjects(
   const std::filesystem::path& path,
   const std::string& expected_source,
   std::span<const ObjectOverride> objects,
   std::string& message,
   std::string& written_source
)
{
   if (objects.empty())
   {
      message = "No property changes to save.";
      return false;
   }
   std::ifstream input(path, std::ios::binary);
   if (!input)
   {
      message = "The source map is not an editable local file.";
      return false;
   }
   const std::string original{std::istreambuf_iterator<char>{input}, {}};
   input.close();
   if (original != expected_source)
   {
      message = "The map changed on disk. Refresh the inspector before applying edits.";
      return false;
   }

   tinyxml2::XMLDocument document;
   if (document.Parse(original.data(), original.size()) != tinyxml2::XML_SUCCESS || !document.RootElement())
   {
      message = "The map could not be parsed. No changes were written.";
      return false;
   }
   struct Replacement
   {
      size_t start;
      size_t length;
      std::string text;
   };
   std::vector<Replacement> replacements;
   std::set<std::string> edited_ids;
   for (const auto& changes : objects)
   {
      if (!changes.schema || (changes.element_type != "object" && changes.element_type != "layer") ||
          !edited_ids.insert(changes.element_type + ":" + changes.object_id).second)
      {
         message = "Each edited object needs a unique source ID and schema.";
         return false;
      }
      const auto& object_id = changes.object_id;
      const auto& schema = *changes.schema;
      const auto& overrides = changes.properties;
      auto* object = findObject(document.RootElement(), object_id, changes.element_type);
      if (!object)
      {
         message = "The selected object no longer exists in this map.";
         return false;
      }
      auto* properties = object->FirstChildElement("properties");
      if (!properties)
      {
         properties = document.NewElement("properties");
         object->InsertFirstChild(properties);
      }
      for (const auto& edit : overrides)
      {
         const auto definition = std::ranges::find(schema.properties, edit.name, &PropertyInfo::name);
         if (definition == schema.properties.end() || !MechanismPropertyBindings::validateValue(*definition, edit.value))
         {
            message = "Invalid value for schema property '" + edit.name + "'. No changes were written.";
            return false;
         }
         auto* property = properties->FirstChildElement("property");
         while (property && (!property->Attribute("name") || edit.name != property->Attribute("name")))
         {
            property = property->NextSiblingElement("property");
         }
         if (!property)
         {
            property = document.NewElement("property");
            property->SetAttribute("name", edit.name.c_str());
            properties->InsertEndChild(property);
         }
         // an override belongs to this map object, even if its old value came from a template
         property->SetAttribute("type", std::string{definition->type}.c_str());
         property->DeleteChildren();
         if (definition->type == "string" && edit.value.find_first_of("\n\r") != std::string::npos)
         {
            property->DeleteAttribute("value");
            property->SetText(edit.value.c_str());
         }
         else
         {
            property->SetAttribute("value", edit.value.c_str());
         }
      }

      // replace only the selected object fragment; preserve the rest of the map byte for byte
      size_t line_start = 0;
      for (int line = 1; line < object->GetLineNum(); ++line)
      {
         const auto newline = original.find('\n', line_start);
         if (newline == std::string::npos)
         {
            message = "Could not locate the original object text.";
            return false;
         }
         line_start = newline + 1;
      }
      if (object_id.empty() || !std::ranges::all_of(object_id, [](char c) { return c >= '0' && c <= '9'; }))
      {
         message = "The object has no numeric TMX ID.";
         return false;
      }
      const std::regex tag_pattern{"<" + changes.element_type + "\\b[^>]*\\bid\\s*=\\s*[\"']" + object_id + "[\"'][^>]*>"};
      size_t start = std::string::npos;
      const auto line_end = original.find('\n', line_start);
      const auto remaining = original.substr(line_start);
      for (std::sregex_iterator match(remaining.begin(), remaining.end(), tag_pattern), end; match != end; ++match)
      {
         const auto offset = line_start + static_cast<size_t>(match->position());
         if (line_end != std::string::npos && offset > line_end)
         {
            break;
         }
         if (start != std::string::npos)
         {
            message = "The object text is ambiguous. Reformat the map in Tiled before editing.";
            return false;
         }
         start = offset;
      }
      const auto header_end = original.find('>', start);
      if (start == std::string::npos || header_end == std::string::npos)
      {
         message = "Could not locate the original object tag.";
         return false;
      }
      const auto close = original[header_end - 1] == '/' ? header_end + 1 : original.find("</" + changes.element_type + ">", header_end);
      if (close == std::string::npos)
      {
         message = "Could not locate the original object end.";
         return false;
      }
      const auto end = original[header_end - 1] == '/' ? close : close + changes.element_type.size() + 3;
      tinyxml2::XMLPrinter printer;
      object->Accept(&printer);
      std::string fragment = printer.CStr();
      while (!fragment.empty() && (fragment.back() == '\n' || fragment.back() == '\r'))
      {
         fragment.pop_back();
      }
      replacements.push_back({start, end - start, std::move(fragment)});
   }
   std::ranges::sort(replacements, std::greater{}, &Replacement::start);
   std::string replacement = original;
   for (const auto& edit : replacements)
   {
      replacement.replace(edit.start, edit.length, edit.text);
   }

   const auto temporary = std::filesystem::path{path.string() + ".inspector.tmp"};
   const auto backup = std::filesystem::path{path.string() + ".inspector.bak"};
   if (!writeFile(backup, original) || !writeFile(temporary, replacement))
   {
      message = "Could not write the map backup or temporary file. The map was not replaced.";
      return false;
   }
   // an external editor may have saved while the replacement was being prepared
   std::ifstream current_file(path, std::ios::binary);
   const std::string current{std::istreambuf_iterator<char>{current_file}, {}};
   current_file.close();
   if (current != original)
   {
      message = "The map changed while saving. The external version was not overwritten.";
      return false;
   }
   std::error_code error;
   std::filesystem::rename(temporary, path, error);
   if (error)
   {
      message = "Could not replace the map: " + error.message();
      return false;
   }
   written_source = replacement;
   message = "Saved to TMX without reloading. A .inspector.bak backup was kept. Properties marked 'reload' need a manual level reload.";
   return true;
}

#endif
