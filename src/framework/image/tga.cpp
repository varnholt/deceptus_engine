// basic tga loader

#include "tga.h"

#include <iostream>
#include <fstream>
#include <string>
#include <ostream>


namespace {

   void write(uint8_t val, std::ostream& out)
   {
      out.write(reinterpret_cast<char*>(&val), sizeof(val));
   }

//   void write(int32_t val, std::ostream& out)
//   {
//      out.write(reinterpret_cast<char*>(&val), sizeof(val));
//   }

//   void write(uint32_t val, std::ostream& out)
//   {
//      out.write(reinterpret_cast<char*>(&val), sizeof(val));
//   }

   void write(int16_t val, std::ostream& out)
   {
      out.write(reinterpret_cast<char*>(&val), sizeof(val));
   }

   void write(uint16_t val, std::ostream& out)
   {
      out.write(reinterpret_cast<char*>(&val), sizeof(val));
   }
}


TGAHeader::TGAHeader(uint16_t w, uint16_t h, uint8_t bits)
{
   _image_type = 2; // rgb + no rle
   _width = w;
   _height = h;
   _bpp = bits;
}


void TGAHeader::save(std::ostream& stream)
{
   write(_ident_size, stream);
   write(_cmap_type, stream);
   write(_image_type, stream);
   write(_cmap_start, stream);
   write(_cmap_length, stream);
   write(_cmap_format, stream);
   write(_origin_x, stream);
   write(_origin_y, stream);
   write(_width, stream);
   write(_height, stream);
   write(_bpp, stream);
   write(_descr, stream);

   for (auto i=0; i < _ident_size; i++)
   {
      write(uint8_t(0), stream);
   }
}

int savetga(const std::string& filename, uint32_t* data, int16_t width, int16_t height)
{
   TGAHeader header(width, height, 32);
   std::ofstream stream(filename, std::ios::binary);
   header.save(stream);

   for (int y = 0; y < height; y++)
   {
      auto* src = data + (height - 1 - y) * header._width;
      for (auto x = 0u; x < header._width; x++)
      {
         auto c = src[x];
         uint8_t b = c & 255;
         uint8_t g = c >> 8 & 255;
         uint8_t r = c >> 16 & 255;
         uint8_t a = c >> 24 & 255;

         write(b, stream);
         write(g, stream);
         write(r, stream);
         write(a, stream);
      }
   }

   stream.close();
   return 32;
}

