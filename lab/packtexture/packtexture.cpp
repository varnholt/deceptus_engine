#include "packtexture.h"
#include <array>
#include <iostream>
#include <fstream>
#include <QFileInfo>
#include <QPainter>


void PackTexture::load(const QString& filename)
{
   filename_ = filename;
   image_.load(filename_);
}


void PackTexture::pack()
{
   quads_.clear();
   auto sizeX = static_cast<int>(ceil(image_.width() / static_cast<float>(size_)));
   auto sizeY = static_cast<int>(ceil(image_.height() / static_cast<float>(size_)));
   auto progress = 0;
   auto progressDone = sizeX * sizeY;

   for (auto blockY = 0; blockY < sizeY; blockY++)
   {
      for (auto blockX = 0; blockX < sizeX; blockX++)
      {
         if (updateProgress_)
         {
            updateProgress_(100 * static_cast<int32_t>((static_cast<float>(progress)/progressDone)));
         }

         progress++;

         QImage subImage = image_.copy(
           blockX * size_,
           blockY * size_,
           size_,
           size_
         );

         auto valid = false;
         for (auto yi = 0; yi < size_; yi++)
         {
            for (auto xi = 0; xi < size_; xi++)
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
            q.data_ = subImage;
            q.x_ = blockX * size_;
            q.y_ = blockY * size_;
            q.w_ = size_;
            q.h_ = size_;
            quads_.push_back(q);
         }
      }
   }

   if (updateProgress_)
   {
      updateProgress_(100);
   }
}


void PackTexture::dump()
{
   auto count = quads_.size();

   std::cout << "[x] quad count: " << count << std::endl;
   std::array<int, 5> textureSizes = {512, 1024, 2048, 4096, 8192};

   auto suitable = std::find_if(std::begin(textureSizes), std::end(textureSizes), [&](auto textureSize){
      auto tmp = textureSize / size_;
      tmp *= tmp;
      return (tmp >= count);
   });

   textureSize_ = *suitable;

   std::cout << "[x] picking a " << textureSize_ << "x" << textureSize_ << " texture" << std::endl;

   QImage out(textureSize_, textureSize_, image_.format());
   QPainter painter(&out);

   std::ofstream uvFile(QString("%1.uv").arg(QFileInfo(filename_).baseName()).toStdString());
   auto x = 0;
   auto y = 0;
   auto progress = 0;

   for (auto& quad : quads_)
   {
      QPoint pos(x, y);
      painter.drawImage(pos, quad.data_);
      x += size_;

      if (x == *suitable)
      {
         x = 0;
         y += size_;
      }

      if (updateProgress_)
      {
         updateProgress_(100 * static_cast<int32_t>((static_cast<float>(progress)/quads_.size())));
      }

      uvFile << progress << ";" << quad.x_ << ";" << quad.y_ << ";" << quad.w_ << ";" << quad.h_ << std::endl;
      progress++;
   }

   painter.end();

   out.save(QString("%1_tiles.png").arg(QFileInfo(filename_).baseName()));

   if (updateProgress_)
   {
      updateProgress_(100);
   }
}

