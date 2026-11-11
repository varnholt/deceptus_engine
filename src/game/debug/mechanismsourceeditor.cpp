#include "mechanismsourceeditor.h"

#if defined(DEVELOPMENT_MODE) && !defined(DECEPTUS_VRSFML)

#include "game/debug/mechanism_property_bindings.h"
#include "game/mechanisms/gamemechanismschema.h"
#include "tinyxml2/tinyxml2.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <iterator>
#include <ranges>
#include <regex>
#include <set>
#include <system_error>
#include <utility>

namespace
{
/// \brief locates a map object or layer by its exact id, excluding tileset collision objects.
/// \param parent XML container whose map groups should be searched.
/// \param id numeric TMX source id.
/// \param element_type source namespace, either object or layer.
/// \return non-owning element pointer, or null when the source does not exist.
tinyxml2::XMLElement* findObject(tinyxml2::XMLElement* parent, const std::string& id, const std::string& element_type)
{
   for (auto* child = parent->FirstChildElement(); child; child = child->NextSiblingElement())
   {
      const auto type = std::string_view{child->Name()};
      if (type == element_type && type == "layer")
      {
         if (const auto* layer_id = child->Attribute("id"); layer_id && id == layer_id)
         {
            return child;
         }
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

/// \brief reads a local map into owned bytes and closes the stream before returning.
/// \param path source file to read.
/// \return complete source bytes, or an error if the file cannot be opened or read.
std::expected<std::string, MechanismSourceEditor::WriteError> readFile(const std::filesystem::path& path)
{
   std::ifstream input(path, std::ios::binary);
   if (!input)
   {
      return std::unexpected{MechanismSourceEditor::WriteError{"The source map is not an editable local file."}};
   }
   std::string contents{std::istreambuf_iterator<char>{input}, {}};
   if (input.bad())
   {
      return std::unexpected{MechanismSourceEditor::WriteError{"Could not read the complete source map."}};
   }
   return contents;
}

/// \brief writes all bytes and checks the final stream state after explicitly closing the file.
/// \param path destination file, replaced if it already exists.
/// \param contents borrowed bytes to write.
/// \return true when writing and closing complete without a stream error.
bool writeFile(const std::filesystem::path& path, std::string_view contents)
{
   std::ofstream file(path, std::ios::binary | std::ios::trunc);
   file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
   file.close();
   return !file.fail();
}
}  // namespace

std::expected<MechanismSourceEditor::WriteResult, MechanismSourceEditor::WriteError> MechanismSourceEditor::writeObjects(
   const std::filesystem::path& path,
   std::string_view expected_source,
   std::span<const ObjectOverride> objects
)
{
   if (objects.empty())
   {
      return std::unexpected{WriteError{"No property changes to save."}};
   }
   const auto source = readFile(path);
   if (!source)
   {
      return std::unexpected{source.error()};
   }
   const auto& original = *source;
   if (original != expected_source)
   {
      return std::unexpected{WriteError{"The map changed on disk. Refresh the editor before applying edits."}};
   }

   tinyxml2::XMLDocument document;
   if (document.Parse(original.data(), original.size()) != tinyxml2::XML_SUCCESS || !document.RootElement())
   {
      return std::unexpected{WriteError{"The map could not be parsed. No changes were written."}};
   }
   /// \brief records one non-overlapping source fragment to replace after the batch validates.
   struct Replacement
   {
      size_t start;      //!< byte offset in the original map.
      size_t length;     //!< number of original bytes to replace.
      std::string text;  //!< serialized replacement fragment.
   };
   std::vector<Replacement> replacements;
   std::set<std::string> edited_ids;
   for (const auto& changes : objects)
   {
      if (!changes.schema || (changes.element_type != "object" && changes.element_type != "layer") ||
          !edited_ids.insert(changes.element_type + ":" + changes.object_id).second)
      {
         return std::unexpected{WriteError{"Each edited object needs a unique source ID and schema."}};
      }
      const auto& object_id = changes.object_id;
      const auto& schema = *changes.schema;
      const auto& overrides = changes.properties;
      auto* object = findObject(document.RootElement(), object_id, changes.element_type);
      if (!object)
      {
         return std::unexpected{WriteError{"The selected object no longer exists in this map."}};
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
            return std::unexpected{WriteError{"Invalid value for schema property '" + edit.name + "'. No changes were written."}};
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
            return std::unexpected{WriteError{"Could not locate the original object text."}};
         }
         line_start = newline + 1;
      }
      if (object_id.empty() || !std::ranges::all_of(object_id, [](char c) { return c >= '0' && c <= '9'; }))
      {
         return std::unexpected{WriteError{"The object has no numeric TMX ID."}};
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
            return std::unexpected{WriteError{"The object text is ambiguous. Reformat the map in Tiled before editing."}};
         }
         start = offset;
      }
      const auto header_end = original.find('>', start);
      if (start == std::string::npos || header_end == std::string::npos)
      {
         return std::unexpected{WriteError{"Could not locate the original object tag."}};
      }
      const auto close = original[header_end - 1] == '/' ? header_end + 1 : original.find("</" + changes.element_type + ">", header_end);
      if (close == std::string::npos)
      {
         return std::unexpected{WriteError{"Could not locate the original object end."}};
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
      return std::unexpected{WriteError{"Could not write the map backup or temporary file. The map was not replaced."}};
   }
   // an external editor may have saved while the replacement was being prepared
   const auto current = readFile(path);
   if (!current)
   {
      return std::unexpected{current.error()};
   }
   if (*current != original)
   {
      return std::unexpected{WriteError{"The map changed while saving. The external version was not overwritten."}};
   }
   std::error_code error;
   std::filesystem::rename(temporary, path, error);
   if (error)
   {
      return std::unexpected{WriteError{"Could not replace the map: " + error.message()}};
   }
   return WriteResult{std::move(replacement), backup};
}

#endif
