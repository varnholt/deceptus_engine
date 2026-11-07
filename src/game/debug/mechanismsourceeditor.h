#pragma once

#if defined(DEVELOPMENT_MODE) && !defined(DECEPTUS_VRSFML)

#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

struct MechanismSchema;

/// \brief validates and persists development-editor property overrides in a local TMX map.
namespace MechanismSourceEditor
{
/// \brief names one authored property and the textual value to write.
struct PropertyOverride
{
   std::string name;   //!< property name declared by the source schema.
   std::string value;  //!< value encoded according to the declared Tiled type.
};

/// \brief groups property overrides belonging to one object or layer source.
struct ObjectOverride
{
   std::string object_id;                     //!< numeric TMX source id within its element namespace.
   const MechanismSchema* schema{nullptr};    //!< non-owning schema that must remain valid throughout the write.
   std::vector<PropertyOverride> properties;  //!< authored values to override in this source only.
   std::string element_type{"object"};        //!< source element kind: object or layer.
};

/// \brief returns the committed map bytes and the pre-save backup location.
struct WriteResult
{
   std::string contents;               //!< complete replacement map bytes for source tracking and watcher acknowledgement.
   std::filesystem::path backup_path;  //!< file containing the map as it was before replacement.
};

/// \brief describes a rejected or unsuccessful save operation.
struct WriteError
{
   std::string message;  //!< explanation suitable for displaying in the editor.
};

/// \brief validates a batch and replaces the map after checking for external source changes.
/// \param path local TMX map to edit.
/// \param expected_source exact bytes the editor last loaded or saved; borrowed for this call.
/// \param objects source overrides to apply; schemas must outlive this call.
/// \return committed map contents and backup path, or an error explaining why replacement did not complete.
/// \note unrelated source fragments and shared templates are preserved; successful saves retain a pre-save backup.
[[nodiscard]] std::expected<WriteResult, WriteError>
writeObjects(const std::filesystem::path& path, std::string_view expected_source, std::span<const ObjectOverride> objects);

}  // namespace MechanismSourceEditor

#endif
