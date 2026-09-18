#include "assetsource.h"

#include <fstream>

// The packed archive only exists for desktop shipping builds. Development builds keep reading
// loose files everywhere, and Switch/WASM (DECEPTUS_VRSFML) already present their data tree as a
// normal filesystem through romfs/preload-file, so they take the same loose-file path too.
#if !defined(DEVELOPMENT_MODE) && !defined(DECEPTUS_VRSFML)
#define DECEPTUS_USE_PACKED_ASSETS
#endif

#ifdef DECEPTUS_USE_PACKED_ASSETS
#include <archive.h>
#include <archive_entry.h>
#include <unordered_map>
#endif

namespace AssetSource
{

namespace
{

std::optional<std::string> readFileFromDisk(const std::filesystem::path& path)
{
   std::ifstream file_stream(path, std::ios::binary);
   if (!file_stream.is_open())
   {
      return std::nullopt;
   }

   return std::string{std::istreambuf_iterator<char>(file_stream), std::istreambuf_iterator<char>()};
}

#ifdef DECEPTUS_USE_PACKED_ASSETS

const std::unordered_map<std::string, std::string>& getPackedAssetTable()
{
   static const std::unordered_map<std::string, std::string> packed_asset_table = []()
   {
      std::unordered_map<std::string, std::string> table;

      struct archive* archive_reader = archive_read_new();
      archive_read_support_format_zip(archive_reader);

      if (archive_read_open_filename(archive_reader, "data.pak", 65536) == ARCHIVE_OK)
      {
         struct archive_entry* entry = nullptr;
         while (archive_read_next_header(archive_reader, &entry) == ARCHIVE_OK)
         {
            const std::string entry_path = archive_entry_pathname(entry);
            const auto entry_size = static_cast<size_t>(archive_entry_size(entry));

            std::string entry_data(entry_size, '\0');
            archive_read_data(archive_reader, entry_data.data(), entry_data.size());

            table.emplace(entry_path, std::move(entry_data));
         }
      }

      archive_read_free(archive_reader);
      return table;
   }();

   return packed_asset_table;
}

#endif

}  // namespace

std::optional<std::string> readFile(const std::filesystem::path& path)
{
#ifdef DECEPTUS_USE_PACKED_ASSETS
   if (path.is_absolute())
   {
      return readFileFromDisk(path);
   }

   const auto& packed_asset_table = getPackedAssetTable();
   const auto found_entry = packed_asset_table.find(path.generic_string());
   if (found_entry == packed_asset_table.end())
   {
      return std::nullopt;
   }

   return found_entry->second;
#else
   return readFileFromDisk(path);
#endif
}

bool exists(const std::filesystem::path& path)
{
#ifdef DECEPTUS_USE_PACKED_ASSETS
   if (path.is_absolute())
   {
      return std::filesystem::exists(path);
   }

   const auto& packed_asset_table = getPackedAssetTable();
   return packed_asset_table.find(path.generic_string()) != packed_asset_table.end();
#else
   return std::filesystem::exists(path);
#endif
}

}  // namespace AssetSource
