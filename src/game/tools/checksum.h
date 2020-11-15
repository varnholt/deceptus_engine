#pragma once

#include <filesystem>
#include <fstream>

class Checksum
{
public:
   static uint32_t calcChecksum(std::ifstream& file);
   static uint32_t calcChecksum(const std::filesystem::path& path);

   static uint32_t readChecksum(std::ifstream& path);
   static uint32_t readChecksum(const std::filesystem::path& pat);

   static void writeChecksum(std::ofstream& path, uint32_t sum);
   static void writeChecksum(const std::filesystem::path& path, uint32_t sum);
};

