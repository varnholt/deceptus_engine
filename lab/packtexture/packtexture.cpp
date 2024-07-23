#include "packtexture.h"
#include <QFileInfo>
#include <QPainter>
#include <array>
#include <fstream>
#include <iostream>

#include <QByteArray>
#include <QCryptographicHash>
#include <iostream>

#include <QImage>
#include <iostream>
#include <vector>

// namespace
// {
// // Function to compare two QImages
// bool imagesAreEqual(const QImage& img1, const QImage& img2)
// {
//    if (img1.size() != img2.size() || img1.format() != img2.format())
//    {
//       return false;
//    }

//    for (int y = 0; y < img1.height(); ++y)
//    {
//       const QRgb* row1 = reinterpret_cast<const QRgb*>(img1.constScanLine(y));
//       const QRgb* row2 = reinterpret_cast<const QRgb*>(img2.constScanLine(y));
//       for (int x = 0; x < img1.width(); ++x)
//       {
//          if (row1[x] != row2[x])
//          {
//             return false;
//          }
//       }
//    }

//    return true;
// }

// // Function to check if a QImage is already in the vector
// bool containsImage(const std::vector<QImage>& images, const QImage& image)
// {
//    for (const auto& img : images)
//    {
//       if (imagesAreEqual(img, image))
//       {
//          return true;
//       }
//    }
//    return false;
// }

// }  // namespace

namespace
{
QString calculateSHA256Checksum(const QImage& image)
{
   QCryptographicHash hash(QCryptographicHash::Sha256);

   for (int y = 0; y < image.height(); ++y)
   {
      const QRgb* row_data = reinterpret_cast<const QRgb*>(image.constScanLine(y));
      hash.addData(reinterpret_cast<const char*>(row_data), image.width() * sizeof(QRgb));
   }

   return QString(hash.result().toHex());
}
}  // namespace

namespace
{
// Precomputed CRC32 table
std::array<uint32_t, 256> createCRC32Table()
{
   std::array<uint32_t, 256> table;
   uint32_t polynomial = 0xEDB88320;

   for (uint32_t i = 0; i < 256; ++i)
   {
      uint32_t crc = i;
      for (uint8_t j = 0; j < 8; ++j)
      {
         if (crc & 1)
         {
            crc = (crc >> 1) ^ polynomial;
         }
         else
         {
            crc >>= 1;
         }
      }
      table[i] = crc;
   }

   return table;
}

const std::array<uint32_t, 256>& getCRC32Table()
{
   static const std::array<uint32_t, 256> table = createCRC32Table();
   return table;
}

// Function to calculate CRC32 checksum for a QImage
uint32_t calculateCRC32Checksum(const QImage& image)
{
   const auto& table = getCRC32Table();
   uint32_t crc = 0xFFFFFFFF;

   for (int y = 0; y < image.height(); ++y)
   {
      const QRgb* row_data = reinterpret_cast<const QRgb*>(image.constScanLine(y));
      for (int x = 0; x < image.width(); ++x)
      {
         uint32_t pixel = row_data[x];
         crc = table[(crc ^ (pixel & 0xFF)) & 0xFF] ^ (crc >> 8);
         crc = table[(crc ^ ((pixel >> 8) & 0xFF)) & 0xFF] ^ (crc >> 8);
         crc = table[(crc ^ ((pixel >> 16) & 0xFF)) & 0xFF] ^ (crc >> 8);
         crc = table[(crc ^ ((pixel >> 24) & 0xFF)) & 0xFF] ^ (crc >> 8);
      }
   }

   return crc ^ 0xFFFFFFFF;
}
}  // namespace
namespace
{
quint32 calculateChecksum(const QImage& image)
{
   quint32 checksum = 0;
   for (auto y = 0; y < image.height(); ++y)
   {
      const QRgb* row_data = reinterpret_cast<const QRgb*>(image.constScanLine(y));
      for (auto x = 0; x < image.width(); ++x)
      {
         checksum ^= row_data[x];
      }
   }
   return checksum;
}
}  // namespace

bool PackTexture::load(const QString& filename)
{
   _filename = filename;
   return _image.load(_filename);
}

void PackTexture::pack()
{
   _quads.clear();

   const auto quad_width = static_cast<int>(ceil(_image.width() / static_cast<float>(_size)));
   const auto quad_height = static_cast<int>(ceil(_image.height() / static_cast<float>(_size)));
   const auto progress_done = quad_width * quad_height;
   auto progress = 0;

   for (auto block_y = 0; block_y < quad_height; block_y++)
   {
      for (auto block_x = 0; block_x < quad_width; block_x++)
      {
         if (_update_progress)
         {
            _update_progress(100 * static_cast<int32_t>((static_cast<float>(progress) / progress_done)));
         }

         progress++;

         QImage sub_image = _image.copy(block_x * _size, block_y * _size, _size, _size);

         auto valid = false;
         for (auto yi = 0; yi < _size; yi++)
         {
            for (auto xi = 0; xi < _size; xi++)
            {
               auto rgb = sub_image.pixel(xi, yi);
               if (qAlpha(rgb) != 0)
               {
                  valid = true;
                  break;
               }
            }
         }

         if (!valid)
         {
            continue;
         }

         const auto checksum = calculateCRC32Checksum(sub_image);
         // const auto checksum = calculateSHA256Checksum(sub_image);

         const auto checksum_it = _quads.find(checksum);

         Rect rect;
         rect._x = block_x * _size;
         rect._y = block_y * _size;
         rect._w = _size;
         rect._h = _size;

         if (checksum_it == _quads.end())
         {
            // create a new unique quad
            Quad q;
            q._image_data = sub_image;
            q._rects.push_back(rect);

            _quads[checksum] = q;
         }
         else
         {
            // extend existing quad
            std::cout << "[!] duplicate found: " << checksum /*.toStdString()*/ << std::endl;
            _quads[checksum]._rects.push_back(rect);
         }
      }
   }

   if (_update_progress)
   {
      _update_progress(100);
   }
}

#include <QDir>

void PackTexture::dump()
{
   auto quad_count = _quads.size();

   std::cout << "[x] quad count: " << quad_count << std::endl;
   std::array<int, 7> texture_sizes = {512, 1024, 2048, 4096, 8192};

   auto suitable_texture_size = std::find_if(
      std::begin(texture_sizes),
      std::end(texture_sizes),
      [&](auto texture_size)
      {
         auto tmp = texture_size / _size;
         tmp *= tmp;
         return (tmp >= quad_count);
      }
   );

   if (suitable_texture_size == texture_sizes.end())
   {
      std::cout << "[e] no suitable texture size for given configuration (" << _size << ")" << std::endl;
      return;
   }
   else
   {
      _texture_size = *suitable_texture_size;
   }

   std::cout << "[x] picking a " << _texture_size << "x" << _texture_size << " texture" << std::endl;

   QImage out(_texture_size, _texture_size, _image.format());
   QPainter painter(&out);
   painter.setCompositionMode(QPainter::CompositionMode_Source);

   const auto uv_filename = QString("%1_tiles.uv").arg(QFileInfo(_filename).baseName()).toStdString();
   std::cout << "[x] dumping uvs to: " << uv_filename << std::endl;
   std::ofstream uv_file(uv_filename);
   auto x = 0;
   auto y = 0;
   auto quad_index = 0;

   for (auto& [_, quad] : _quads)
   {
      const QPoint pos(x, y);
      painter.drawImage(pos, quad._image_data);
      x += _size;

      // go to next row in target texture
      if (x == _texture_size)
      {
         x = 0;
         y += _size;
      }

      if (_update_progress)
      {
         _update_progress(100 * static_cast<int32_t>((static_cast<float>(quad_index) / _quads.size())));
      }

      for (auto& rect : quad._rects)
      {
         uv_file << quad_index << ";" << rect._x << ";" << rect._y << ";" << rect._w << ";" << rect._h << std::endl;
      }

      quad_index++;
   }

   painter.end();

   uv_file.close();

   const auto texture_filename = QString("%1_tiles.png").arg(QFileInfo(_filename).baseName());
   std::cout << "[x] dumping texture to: " << texture_filename.toStdString() << std::endl;

   if (!out.save(texture_filename))
   {
      std::cout << "[e] saving failed" << std::endl;
   }

   if (_update_progress)
   {
      _update_progress(100);
   }

   std::cout << "[x] done!" << std::endl;
}
