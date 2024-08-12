#include "checksum.h"

#include <cstdint>

uint32_t Checksum::calcChecksum(std::ifstream& file)
{
   uint32_t sum = 0;
   uint32_t word = 0;

   while (file.read(reinterpret_cast<char*>(&word), sizeof(word)))
   {
      sum += word;
   }

   return sum;
}

uint32_t Checksum::calcChecksum(const std::filesystem::path& path)
{
   std::ifstream ifs(path, std::ifstream::binary);
   uint32_t sum = calcChecksum(ifs);
   ifs.close();
   return sum;
}

uint32_t Checksum::readChecksum(std::ifstream& file)
{
   uint32_t word = 0;
   file.read(reinterpret_cast<char*>(&word), sizeof(word));
   return word;
}

uint32_t Checksum::readChecksum(const std::filesystem::path& path)
{
   std::ifstream ifs(path, std::ifstream::binary);
   uint32_t sum = readChecksum(ifs);
   ifs.close();
   return sum;
}

void Checksum::writeChecksum(std::ofstream& ofs, uint32_t word)
{
   ofs.write(reinterpret_cast<char*>(&word), sizeof(word));
}

void Checksum::writeChecksum(const std::filesystem::path& path, uint32_t sum)
{
   std::ofstream ofs;
   ofs.open(path, std::ofstream::binary);
   writeChecksum(ofs, sum);
}
