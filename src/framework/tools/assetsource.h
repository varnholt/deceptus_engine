#pragma once

#include <filesystem>
#include <optional>
#include <string>

///
/// \brief Reads asset files, transparently switching between loose files and a packed archive.
///
/// In development builds this is a thin pass-through to disk, so behavior stays unchanged while
/// iterating on assets. In shipping builds, relative paths under the data tree (every literal like
/// "data/sprites/foo.png" throughout the codebase) are instead served from an in-memory table
/// built once from data.pak next to the executable; absolute paths (the writable settings/save
/// tree resolved through GamePaths) always keep reading straight from disk, since that tree is
/// never packed.
///
namespace AssetSource
{
///
/// \brief Reads the full contents of `path`.
/// \param path File path, relative (data tree) or absolute (writable user data).
/// \return File contents, or std::nullopt when the file could not be read.
///
std::optional<std::string> readFile(const std::filesystem::path& path);

///
/// \brief Checks whether `path` can be read.
/// \param path File path, relative (data tree) or absolute (writable user data).
/// \return true when the file exists and is readable.
///
bool exists(const std::filesystem::path& path);

}  // namespace AssetSource
