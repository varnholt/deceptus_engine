#pragma once

#include <filesystem>
#include <fstream>

///
/// \brief Computes, reads, and writes 32-bit checksums for binary data.
///
class Checksum
{
public:
   ///
   /// \brief Sums all 32-bit words from an open binary stream.
   /// \param file Input stream positioned at the checksum source data.
   /// \return Accumulated checksum value.
   ///
   static uint32_t calcChecksum(std::ifstream& file);

   ///
   /// \brief Opens a file and computes its checksum.
   /// \param path File path to read.
   /// \return Accumulated checksum value.
   ///
   static uint32_t calcChecksum(const std::filesystem::path& path);

   ///
   /// \brief Reads one 32-bit checksum word from an open binary stream.
   /// \param file Input stream positioned at the checksum word.
   /// \return Checksum word value.
   ///
   static uint32_t readChecksum(std::ifstream& file);

   ///
   /// \brief Opens a file and reads its first checksum word.
   /// \param path File path to read.
   /// \return Checksum word value.
   ///
   static uint32_t readChecksum(const std::filesystem::path& path);

   ///
   /// \brief Writes one 32-bit checksum word to an open binary stream.
   /// \param file Output stream to write into.
   /// \param sum Checksum value to write.
   ///
   static void writeChecksum(std::ofstream& file, uint32_t sum);

   ///
   /// \brief Opens a file and writes one checksum word.
   /// \param path File path to write.
   /// \param sum Checksum value to write.
   ///
   static void writeChecksum(const std::filesystem::path& path, uint32_t sum);
};
