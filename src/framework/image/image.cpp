#include "image.h"
#include "tga.h"

#include <memory.h>
#include <algorithm>
#include <cmath>

// construct image from file
Image::Image(const std::string& filename)
{
   load(filename);
}

// construct references
Image::Image(const Image& image) : _data(image.getData()), _width(image.getWidth()), _height(image.getHeight())
{
}

// assignment operator: create reference
const Image& Image::operator=(const Image& image)
{
   if (this != &image)
   {
      _width = image.getWidth();
      _height = image.getHeight();
      _data = image.getData();
   }

   return *this;
}

void Image::init(int32_t x, int32_t y)
{
   _data.resize(x * y, 0);
   _width = x;
   _height = y;
}

// save image to file
void Image::save(const std::string& filename) const
{
   savetga(filename, const_cast<uint32_t*>(getData().data()), static_cast<int16_t>(getWidth()), static_cast<int16_t>(getHeight()));
}

// load image from file
void Image::load(const std::string& /*filename*/)
{
   // // data is not referenced by another object? delete it.
   // if (!copyRef())
   //    discard();
   //
   //
   // if (loadtga(filename, (void**)&mData, &mWidth, &mHeight))
   // {
   //    char name[256];
   //    strcpy(name, filename);
   //    strcat(name, ".tga");
   //    FileStream stream;
   //    if (stream.open(name))
   //    {
   //       _path = stream.getPath();
   //       _filename = filename;
   //       stream.close();
   //    }
   // }
}

// get image width
int32_t Image::getWidth() const
{
   return _width;
}

// get image height
int32_t Image::getHeight() const
{
   return _height;
}

// get scanline
uint32_t* Image::getScanline(int32_t y) const
{
   return const_cast<uint32_t*>(_data.data()) + y * _width;
}

// get data
const std::vector<uint32_t>& Image::getData() const
{
   return _data;
}

const std::string& Image::path() const
{
   return _path;
}

const std::string& Image::filename() const
{
   return _filename;
}

uint32_t Image::getPixel(float u, float v) const
{
   const auto x = static_cast<int32_t>(floor(u * (_width - 1)));
   const auto y = static_cast<int32_t>(floor(v * (_height - 1)));

   return _data[y * _width + x];
}

// half resolution
Image Image::downsample() const
{
   const auto nx = _width >> 1;
   const auto ny = _height >> 1;

   Image image;
   image.init(nx, ny);

   for (auto y = 0; y < ny; y++)
   {
      auto dst = image.getScanline(y);

      auto src1 = getScanline(y * 2);
      auto src2 = getScanline(y * 2 + 1);

      for (int32_t x = 0; x < nx; x++)
      {
         uint32_t c1 = *src1++;
         uint32_t c2 = *src1++;
         uint32_t c3 = *src2++;
         uint32_t c4 = *src2++;

         int32_t a = ((c1 >> 24 & 0xff) + (c2 >> 24 & 0xff) + (c3 >> 24 & 0xff) + (c4 >> 24 & 0xff)) >> 2;
         int32_t r = ((c1 >> 16 & 0xff) + (c2 >> 16 & 0xff) + (c3 >> 16 & 0xff) + (c4 >> 16 & 0xff)) >> 2;
         int32_t g = ((c1 >> 8 & 0xff) + (c2 >> 8 & 0xff) + (c3 >> 8 & 0xff) + (c4 >> 8 & 0xff)) >> 2;
         int32_t b = ((c1 & 0xff) + (c2 & 0xff) + (c3 & 0xff) + (c4 & 0xff)) >> 2;

         *dst++ = (a << 24) + (r << 16) + (g << 8) + b;
      }
   }

   return image;
}

// linear blend between c1 & c2
uint32_t blend(uint32_t c1, uint32_t c2, unsigned char f)
{
   unsigned char a = (c1 >> 24 & 0xff) + (((c2 >> 24 & 0xff) - (c1 >> 24 & 0xff)) * f >> 8);
   unsigned char r = (c1 >> 16 & 0xff) + (((c2 >> 16 & 0xff) - (c1 >> 16 & 0xff)) * f >> 8);
   unsigned char g = (c1 >> 8 & 0xff) + (((c2 >> 8 & 0xff) - (c1 >> 8 & 0xff)) * f >> 8);
   unsigned char b = (c1 & 0xff) + (((c2 & 0xff) - (c1 & 0xff)) * f >> 8);

   return (a << 24) | (r << 16) | (g << 8) | b;
}

// create scaled version of given image
void Image::scaled(const Image& image) const
{
   const auto w = image.getWidth();
   const auto h = image.getHeight();

   const auto dx = (w << 16) / _width;
   const auto dy = (h << 16) / _height;

   auto iy = 0;

   for (auto dstY = 0; dstY < _height; dstY++)
   {
      auto y = iy >> 16;
      auto sy = static_cast<unsigned char>(iy >> 8 & 0xff);

      auto dst = getScanline(dstY);
      uint32_t *src1, *src2;

      src1 = image.getScanline(y);

      if (y == h - 1)
      {
         src2 = image.getScanline(y);
      }
      else
      {
         src2 = image.getScanline(y + 1);  // don't exceed image boundaries
      }

      int32_t ix = 0;
      for (int32_t dstX = 0; dstX < _width - 1; dstX++)
      {
         auto x = ix >> 16;
         auto sx = static_cast<unsigned char>(ix >> 8 & 0xff);

         auto c1 = src1[x];
         auto c2 = src1[x + 1];
         auto c3 = src2[x];
         auto c4 = src2[x + 1];

         c1 = blend(c1, c2, sx);
         c2 = blend(c3, c4, sx);

         dst[dstX] = blend(c1, c2, sy);

         ix += dx;
      }
      dst[_width - 1] = blend(src1[w - 1], src2[w - 1], sy);

      iy += dy;
   }
}

void Image::premultiplyAlpha()
{
   for (auto y = 0; y < _height; y++)
   {
      auto dst = getScanline(y);

      for (auto x = 0; x < _width; x++)
      {
         auto c1 = dst[x];

         unsigned char a = (c1 >> 24 & 0xff);

         if (a != 255)
         {
            unsigned char r = (c1 >> 16 & 0xff);
            unsigned char g = (c1 >> 8 & 0xff);
            unsigned char b = (c1 & 0xff);

            r = (r * a) >> 8;
            g = (g * a) >> 8;
            b = (b * a) >> 8;

            dst[x] = (a << 24) + (r << 16) + (g << 8) + b;
         }
      }
   }
}

void Image::minimum(const Image& image)
{
   const auto width = std::min<int32_t>(_width, image.getWidth());
   const auto height = std::min<int32_t>(_height, image.getHeight());

   for (auto y = 0; y < height; y++)
   {
      auto* dst = getScanline(y);
      const auto* src = image.getScanline(y);

      for (auto x = 0; x < width; x++)
      {
         uint32_t c1 = src[x];
         uint32_t c2 = dst[x];

         unsigned char a1 = (c1 >> 24 & 0xff);
         unsigned char r1 = (c1 >> 16 & 0xff);
         unsigned char g1 = (c1 >> 8 & 0xff);
         unsigned char b1 = (c1 & 0xff);

         unsigned char a2 = (c2 >> 24 & 0xff);
         unsigned char r2 = (c2 >> 16 & 0xff);
         unsigned char g2 = (c2 >> 8 & 0xff);
         unsigned char b2 = (c2 & 0xff);

         if (a2 < a1)
            a1 = a2;
         if (r2 < r1)
            r1 = r2;
         if (g2 < g1)
            g1 = g2;
         if (b2 < b1)
            b1 = b2;

         dst[x] = (a1 << 24) + (r1 << 16) + (g1 << 8) + b1;
      }
   }
}

void Image::clear(uint32_t argb)
{
   for (int32_t y = 0; y < _height; y++)
   {
      auto dst = getScanline(y);

      for (int32_t x = 0; x < _width; x++)
      {
         dst[x] = argb;
      }
   }
}

void Image::copy(int32_t posX, int32_t posY, const Image& image, int32_t replicate)
{
   const auto width = std::min<int32_t>(_width - posX, image.getWidth());
   const auto height = std::min<int32_t>(_height - posY, image.getHeight());

   const auto endx = _width - posX;
   const auto endy = _height - posY;

   for (auto y = 0; y < height; y++)
   {
      auto dst = getScanline(y + posY) + posX;
      const auto* src = image.getScanline(y);

      for (auto x = 0; x < width; x++)
      {
         dst[x] = src[x];
      }

      // replicate last pixel
      for (int32_t x = 0; x < endx - width && x < replicate; x++)
      {
         dst[width + x] = src[width - 1];
      }
   }

   // replicate last scanline
   if (replicate > 0)
   {
      auto src = getScanline(height - 1);

      for (int32_t y = 0; y < endy - height && y < replicate; y++)
      {
         uint32_t* dst = getScanline(y + height) + posX;
         memcpy(dst, src, endx * 4);
      }
   }
}

uint32_t calcNormal(int32_t z, uint32_t x0, uint32_t x1, uint32_t y0, uint32_t y1)
{
   // height= red + green + blue;
   x0 = (x0 >> 16 & 255) + (x0 >> 8 & 255) + (x0 & 255);
   x1 = (x1 >> 16 & 255) + (x1 >> 8 & 255) + (x1 & 255);
   y0 = (y0 >> 16 & 255) + (y0 >> 8 & 255) + (y0 & 255);
   y1 = (y1 >> 16 & 255) + (y1 >> 8 & 255) + (y1 & 255);

   int32_t x = static_cast<int32_t>(x0 - x1);
   int32_t y = static_cast<int32_t>(y0 - y1);

   const auto magnitude = x * x + y * y + z * z;
   const auto t = static_cast<float>(128.0 / sqrt(static_cast<double>(magnitude)));

   x = static_cast<int32_t>(128 + x * t);
   if (x < 0)
      x = 0;
   if (x > 255)
      x = 255;
   y = static_cast<int32_t>(128 - y * t);
   if (y < 0)
      y = 0;
   if (y > 255)
      y = 255;
   z = static_cast<int32_t>(128 + z * t);
   if (z < 0)
      z = 0;
   if (z > 255)
      z = 255;

   return (255 << 24) | (x << 16) | (y << 8) | z;
}

uint32_t* Image::buildNormalMap(int32_t z)
{
   auto src = _data.data();

   auto src0 = _data.data() + (_height - 1) * _width;
   auto src1 = _data.data();
   auto src2 = _data.data() + _width;

   auto dst = new uint32_t[_width * _height];

   for (auto y = 0; y < _height; y++)
   {
      dst[0] = calcNormal(z, src1[_width - 1], src1[1], src0[0], src2[0]);

      for (int32_t x = 1; x < _width - 1; x++)
      {
         dst[x] = calcNormal(z, src1[x - 1], src1[x + 1], src0[x], src2[x]);
      }

      dst[_width - 1] = calcNormal(z, src1[_width - 2], src1[0], src0[_width - 1], src2[_width - 1]);

      dst += _width;

      src0 = src1;
      src1 = src2;

      if (y < _height - 2)
      {
         src2 += _width;
      }
      else
      {
         src2 = _data.data();
      }
   }

   delete[] src;
   return dst;
}

uint32_t* Image::buildDeltaMap()
{
   auto temp = _data.data();

   auto src0 = _data.data() + (_height - 1) * _width;
   auto src1 = _data.data();
   auto src2 = _data.data() + _width;

   auto dst = new uint32_t[_width * _height];

   const auto s = 2;

   for (auto y = 0; y < _height; y++)
   {
      dst[0] =
         ((128 + (src1[_width - 1] & 0xff) * s - (src1[1] & 0xff) * s) << 16) | ((128 + (src0[0] & 0xff) * s - (src2[0] & 0xff) * s) << 8);

      for (auto x = 1; x < _width - 1; x++)
      {
         dst[x] = ((128 + (src1[x - 1] & 0xff) * s - (src1[x + 1] & 0xff) * s) << 16) |
                  ((128 + (src0[x] & 0xff) * s - (src2[x] & 0xff) * s) << 8);
      }

      dst[_width - 1] = ((128 + (src1[_width - 2] & 0xff) * s - (src1[0] & 0xff) * s) << 16) |
                        ((128 + (src0[_width - 1] & 0xff) * s - (src2[_width - 1] & 0xff) * s) << 8);

      dst += _width;

      src0 = src1;
      src1 = src2;

      if (y < _height - 2)
      {
         src2 += _width;
      }
      else
      {
         src2 = _data.data();
      }
   }

   delete[] temp;
   return dst;
}
