#include "image.h"
#include "tga.h"

#include <algorithm>
#include <memory.h>
#include <math.h>


// construct image from file
Image::Image(const std::string &filename)
{
   load(filename);
}

// construct references
Image::Image(const Image& image)
: mData(image.getData())
, mWidth(image.getWidth())
, mHeight(image.getHeight())
{
}

// assignment operator: create reference
const Image& Image::operator = (const Image& image)
{
   if (this != &image)
   {
       mWidth = image.getWidth();
       mHeight = image.getHeight();
       mData = image.getData();
   }

   return *this;
}

void Image::init(int32_t x, int32_t y)
{
   mData.resize(x * y, 0);
   mWidth = x;
   mHeight = y;
}

// save image to file
void Image::save(const std::string& filename) const
{
   savetga(filename, const_cast<uint32_t*>(getData().data()), getWidth(), getHeight());
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
   //       mPath= stream.getPath();
   //       mFilename= filename;
   //       stream.close();
   //    }
   // }
}

// get image width
int32_t Image::getWidth() const
{
   return mWidth;
}

// get image height
int32_t Image::getHeight() const
{
   return mHeight;
}

// get scanline
uint32_t* Image::getScanline(int32_t y) const
{
   return const_cast<uint32_t*>(mData.data()) + y * mWidth;
}

// get data
const std::vector<uint32_t>& Image::getData() const
{
   return mData;
}

const std::string& Image::path() const
{
   return mPath;
}

const std::string& Image::filename() const
{
   return mFilename;
}

uint32_t Image::getPixel(float u, float v) const
{
    int32_t x= (int) floor(u*(mWidth-1));
    int32_t y= (int) floor(v*(mHeight-1));

    return mData[y*mWidth+x];
}

// halve resolution
Image Image::downsample() const
{
   int32_t nx= mWidth>>1;
   int32_t ny= mHeight>>1;

   Image image;
   image.init(nx, ny);

   for (int32_t y=0;y<ny;y++)
   {
      uint32_t *dst= image.getScanline(y);

      uint32_t *src1= getScanline(y*2);
      uint32_t *src2= getScanline(y*2+1);

      for (int32_t x=0;x<nx;x++)
      {
         uint32_t c1= *src1++;
         uint32_t c2= *src1++;
         uint32_t c3= *src2++;
         uint32_t c4= *src2++;

         int32_t a= ((c1>>24&0xff) + (c2>>24&0xff) + (c3>>24&0xff) + (c4>>24&0xff)) >> 2;
         int32_t r= ((c1>>16&0xff) + (c2>>16&0xff) + (c3>>16&0xff) + (c4>>16&0xff)) >> 2;
         int32_t g= ((c1>> 8&0xff) + (c2>> 8&0xff) + (c3>> 8&0xff) + (c4>> 8&0xff)) >> 2;
         int32_t b= ((c1    &0xff) + (c2    &0xff) + (c3    &0xff) + (c4    &0xff)) >> 2;

         *dst++= (a<<24)+(r<<16)+(g<<8)+b;
      }
   }

   return image;
}

// linear blend between c1 & c2
uint32_t blend(uint32_t c1, uint32_t c2, unsigned char f)
{
   unsigned char a= (c1>>24&0xff) + (( (c2>>24&0xff) - (c1>>24&0xff) ) * f >> 8);
   unsigned char r= (c1>>16&0xff) + (( (c2>>16&0xff) - (c1>>16&0xff) ) * f >> 8);
   unsigned char g= (c1>>8 &0xff) + (( (c2>>8 &0xff) - (c1>>8 &0xff) ) * f >> 8);
   unsigned char b= (c1    &0xff) + (( (c2    &0xff) - (c1    &0xff) ) * f >> 8);

   return (a<<24)|(r<<16)|(g<<8)|b;
}

// create scaled version of given image
void Image::scaled(const Image& image) const
{
   int32_t w= image.getWidth();
   int32_t h= image.getHeight();

   int32_t dx= (w << 16) / mWidth;
   int32_t dy= (h << 16) / mHeight;

   int32_t iy= 0;
   for (int32_t dstY=0; dstY<mHeight; dstY++)
   {
      int32_t y= iy >> 16;
      int32_t sy= iy >> 8 & 0xff;

      uint32_t *dst= getScanline(dstY);
      uint32_t *src1, *src2;
      src1= image.getScanline(y);
      if (y==h-1)
         src2= image.getScanline(y);
      else
         src2= image.getScanline(y+1); // don't exceed image boundaries

      int32_t ix= 0;
      for (int32_t dstX=0; dstX<mWidth-1; dstX++)
      {
         int32_t x= ix >> 16;
         int32_t sx= ix >> 8 & 0xff;

         uint32_t c1= src1[x];
         uint32_t c2= src1[x+1];
         uint32_t c3= src2[x];
         uint32_t c4= src2[x+1];

         c1= blend(c1, c2, sx);
         c2= blend(c3, c4, sx);

         dst[dstX]= blend(c1, c2, sy);

         ix+=dx;
      }
      dst[mWidth-1]= blend(src1[w-1], src2[w-1], sy);

      iy+=dy;
   }
}

void Image::premultiplyAlpha()
{
   for (int32_t y=0;y<mHeight;y++)
   {
      uint32_t *dst= getScanline(y);

      for (int32_t x=0;x<mWidth;x++)
      {
         uint32_t c1= dst[x];

         unsigned char a= (c1>>24&0xff);
         if (a != 255)
         {
            unsigned char r= (c1>>16&0xff);
            unsigned char g= (c1>> 8&0xff);
            unsigned char b= (c1    &0xff);

            r= (r*a)>>8;
            g= (g*a)>>8;
            b= (b*a)>>8;

            dst[x]= (a<<24)+(r<<16)+(g<<8)+b;
         }
      }
   }
}

void Image::minimum(const Image& image)
{
   int32_t width= std::min<int>(mWidth, image.getWidth());
   int32_t height= std::min<int>(mHeight, image.getHeight());

   for (int32_t y=0; y<height; y++)
   {
      uint32_t *dst= getScanline(y);
      uint32_t *src= image.getScanline(y);

      for (int32_t x=0; x<width; x++)
      {
         uint32_t c1= src[x];
         uint32_t c2= dst[x];

         unsigned char a1= (c1>>24&0xff);
         unsigned char r1= (c1>>16&0xff);
         unsigned char g1= (c1>> 8&0xff);
         unsigned char b1= (c1    &0xff);

         unsigned char a2= (c2>>24&0xff);
         unsigned char r2= (c2>>16&0xff);
         unsigned char g2= (c2>> 8&0xff);
         unsigned char b2= (c2    &0xff);

         if (a2<a1) a1= a2;
         if (r2<r1) r1= r2;
         if (g2<g1) g1= g2;
         if (b2<b1) b1= b2;

         dst[x]= (a1<<24)+(r1<<16)+(g1<<8)+b1;
      }
   }
}

void Image::clear(uint32_t argb)
{
   for (int32_t y=0; y<mHeight; y++)
   {
      uint32_t *dst= getScanline(y);
      for (int32_t x=0; x<mWidth; x++)
         dst[x]= argb;
   }
}
void Image::copy(int32_t posX, int32_t posY, const Image& image, int32_t replicate)
{
   int32_t width= std::min<int>(mWidth - posX, image.getWidth());
   int32_t height= std::min<int>(mHeight - posY, image.getHeight());

   int32_t endx= mWidth - posX;
   int32_t endy= mHeight - posY;

   for (int32_t y=0; y<height; y++)
   {
      uint32_t *dst= getScanline(y + posY) + posX;
      uint32_t *src= image.getScanline(y);

      for (int32_t x=0; x<width; x++)
      {
         dst[x]= src[x];
      }

      // replicate last pixel
      for (int32_t x=0; x<endx-width && x<replicate; x++)
         dst[width+x]= src[width-1];
   }

   // replicate last scanline
   if (replicate>0)
   {
       uint32_t *src= getScanline( height-1 );
       for (int32_t y=0; y<endy-height && y<replicate; y++)
       {
          uint32_t *dst= getScanline(y+height) + posX;
          memcpy(dst, src, endx*4);
       }
    }
}


uint32_t calcNormal(int32_t z, uint32_t x0, uint32_t x1, uint32_t y0, uint32_t y1)
{
   // height= red + green + blue;
   x0= (x0 >> 16 & 255) + (x0 >> 8 & 255) + (x0 & 255);
   x1= (x1 >> 16 & 255) + (x1 >> 8 & 255) + (x1 & 255);
   y0= (y0 >> 16 & 255) + (y0 >> 8 & 255) + (y0 & 255);
   y1= (y1 >> 16 & 255) + (y1 >> 8 & 255) + (y1 & 255);

   int32_t x= (x0-x1);
   int32_t y= (y0-y1);

   int32_t mag= x*x + y*y + z*z;
   float t = static_cast<float>(128.0 / sqrt( (double)mag));
   x = static_cast<int32_t>(128 + x * t); if (x < 0) x = 0; if (x > 255) x = 255;
   y = static_cast<int32_t>(128 - y * t); if (y < 0) y = 0; if (y > 255) y = 255;
   z = static_cast<int32_t>(128 + z * t); if (z < 0) z = 0; if (z > 255) z = 255;

   return (255<<24) | (x<<16) | (y<<8) | z;
}


uint32_t* Image::buildNormalMap(int32_t z)
{
   uint32_t* src = mData.data();

   uint32_t* src0 = mData.data() + (mHeight - 1) * mWidth;
   uint32_t* src1 = mData.data();
   uint32_t* src2 = mData.data() + mWidth;

   uint32_t* dst = new uint32_t[mWidth * mHeight];

   for (int32_t y = 0; y < mHeight; y++)
   {
      dst[0] = calcNormal(z, src1[mWidth - 1], src1[1], src0[0], src2[0]);

      for (int32_t x = 1; x < mWidth - 1; x++)
      {
         dst[x] = calcNormal(z, src1[x - 1], src1[x+1], src0[x], src2[x]);
      }

      dst[mWidth - 1] = calcNormal(z, src1[mWidth - 2], src1[0], src0[mWidth - 1], src2[mWidth - 1]);

      dst += mWidth;

      src0 = src1;
      src1 = src2;

      if (y < mHeight - 2)
      {
         src2 += mWidth;
      }
      else
      {
         src2 = mData.data();
      }
   }

   delete[] src;
   return dst;
}


uint32_t* Image::buildDeltaMap()
{
   uint32_t* temp = mData.data();

   uint32_t* src0 = mData.data() + (mHeight-1)*mWidth;
   uint32_t* src1 = mData.data();
   uint32_t* src2 = mData.data() + mWidth;

   uint32_t* dst = new uint32_t[mWidth * mHeight];

   int32_t s = 2;

   for (int32_t y = 0; y < mHeight; y++)
   {
      dst[0]=
            ((128 + (src1[mWidth-1] & 0xff) * s - (src1[1]&0xff)*s) << 16)
          | ((128 + (src0[0]        & 0xff) * s - (src2[0]&0xff)*s) << 8);

      for (int32_t x = 1; x < mWidth - 1; x++)
      {
         dst[x]=
              ((128 + (src1[x-1] & 0xff) * s - (src1[x+1] & 0xff) * s) << 16)
            | ((128 + (src0[x]   & 0xff) * s - (src2[x]   & 0xff) * s) << 8);
      }

      dst[mWidth - 1]=
           ((128 + (src1[mWidth - 2] & 0xff) * s - (src1[0]          & 0xff) * s) << 16)
         | ((128 + (src0[mWidth - 1] & 0xff) * s - (src2[mWidth - 1] & 0xff) * s) << 8);

      dst += mWidth;

      src0 = src1;
      src1 = src2;

      if (y < mHeight - 2)
      {
         src2 += mWidth;
      }
      else
      {
         src2 = mData.data();
      }
   }

   delete[] temp;
   return dst;
}

