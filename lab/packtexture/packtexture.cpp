#include "packtexture.h"
#include <array>
#include <iostream>
#include <fstream>
#include <QFileInfo>
#include <QPainter>


bool PackTexture::load(const QString& filename)
{
   mFilename = filename;
   return mImage.load(mFilename);
}


void PackTexture::pack()
{
   mQuads.clear();
   auto sizeX = static_cast<int>(ceil(mImage.width() / static_cast<float>(mSize)));
   auto sizeY = static_cast<int>(ceil(mImage.height() / static_cast<float>(mSize)));
   auto progress = 0;
   auto progressDone = sizeX * sizeY;

   for (auto blockY = 0; blockY < sizeY; blockY++)
   {
      for (auto blockX = 0; blockX < sizeX; blockX++)
      {
         if (mUpdateProgress)
         {
            mUpdateProgress(100 * static_cast<int32_t>((static_cast<float>(progress)/progressDone)));
         }

         progress++;

         QImage subImage = mImage.copy(
           blockX * mSize,
           blockY * mSize,
           mSize,
           mSize
         );

         auto valid = false;
         for (auto yi = 0; yi < mSize; yi++)
         {
            for (auto xi = 0; xi < mSize; xi++)
            {
               auto rgb = subImage.pixel(xi, yi);
               if (qAlpha(rgb) != 0)
               {
                  valid = true;
                  break;
               }
            }
         }

         if (valid)
         {
            Quad q;
            q.mData = subImage;
            q.mX = blockX * mSize;
            q.mY = blockY * mSize;
            q.mW = mSize;
            q.mH = mSize;
            mQuads.push_back(q);
         }
      }
   }

   if (mUpdateProgress)
   {
      mUpdateProgress(100);
   }
}


void PackTexture::dump()
{
   auto count = mQuads.size();

   std::cout << "[x] quad count: " << count << std::endl;
   std::array<int, 5> textureSizes = {512, 1024, 2048, 4096, 8192};

   auto suitable = std::find_if(std::begin(textureSizes), std::end(textureSizes), [&](auto textureSize){
      auto tmp = textureSize / mSize;
      tmp *= tmp;
      return (tmp >= count);
   });

   mTextureSize = *suitable;

   std::cout << "[x] picking a " << mTextureSize << "x" << mTextureSize << " texture" << std::endl;

   QImage out(mTextureSize, mTextureSize, mImage.format());
   QPainter painter(&out);
   painter.setCompositionMode(QPainter::CompositionMode_Source);

   const auto uvFilename = QString("%1_tiles.uv").arg(QFileInfo(mFilename).baseName()).toStdString();
   std::cout << "[x] dumping uvs to: " << uvFilename << std::endl;
   std::ofstream uvFile(uvFilename);
   auto x = 0;
   auto y = 0;
   auto progress = 0;

   for (auto& quad : mQuads)
   {
      QPoint pos(x, y);
      painter.drawImage(pos, quad.mData);
      x += mSize;

      if (x == *suitable)
      {
         x = 0;
         y += mSize;
      }

      if (mUpdateProgress)
      {
         mUpdateProgress(100 * static_cast<int32_t>((static_cast<float>(progress)/mQuads.size())));
      }

      uvFile << progress << ";" << quad.mX << ";" << quad.mY << ";" << quad.mW << ";" << quad.mH << std::endl;
      progress++;
   }

   painter.end();

   const auto textureFilename = QString("%1_tiles.png").arg(QFileInfo(mFilename).baseName());
   std::cout << "[x] dumping texture to: " << textureFilename.toStdString() << std::endl;
   out.save(textureFilename);

   if (mUpdateProgress)
   {
      mUpdateProgress(100);
   }

   std::cout << "[x] done!" << std::endl;
}

