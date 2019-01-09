#pragma once

#include <stdint.h>
#include <string>

int savetga(const std::string& filename, uint32_t *data, int32_t width, int32_t height);

struct TGAHeader
{
   TGAHeader() = default;
   TGAHeader(uint16_t w, uint16_t h, uint8_t bits);
   void save(std::ostream& stream);

   uint8_t identsize = 0;    // size of ID field that follows 18 byte header (0 usually)
   uint8_t cmaptype = 0;     // type of colour map 0=none, 1=has palette
   uint8_t imagetype = 0;    // type of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed

   int16_t cmapstart = 0;    // first colour map entry in palette
   int16_t cmaplength = 0;   // number of colours in palette
   uint8_t cmapformat = 0;   // number of bits per palette entry 15,16,24,32

   int16_t originx = 0;      // image x origin
   int16_t originy = 0;      // image y origin

   uint16_t width = 0;
   uint16_t height = 0;
   uint8_t bpp = 0;
   uint8_t descr = 0;
   bool rle = false;
};

