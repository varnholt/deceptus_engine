#include "packtexture.h"

#include <QApplication>
#include <QByteArray>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QString>
#include <array>
#include <fstream>
#include <iostream>
#include <vector>

namespace
{
// precomputed CRC32 table
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
            _log(qApp->tr("duplicate block found at %1x%2").arg(block_x).arg(block_y));
            _quads[checksum]._rects.push_back(rect);
         }
      }
   }

   if (_update_progress)
   {
      _update_progress(100);
   }
}

void PackTexture::dump()
{
   auto quad_count = _quads.size();

   _log(qApp->tr("quad count: %1").arg(quad_count));
   std::array<int, 5> candidate_texture_widths = {512, 1024, 2048, 4096, 8192};

   // the width is chosen as though the atlas were still square, which is what keeps every atlas
   // packed before the height was trimmed laid out exactly as it was: the runtime derives a quad's
   // column from (index * quad_width) % texture_width, so changing the width would renumber every
   // quad and invalidate the uv file that ships next to the texture
   auto suitable_texture_width = std::find_if(
      std::begin(candidate_texture_widths),
      std::end(candidate_texture_widths),
      [&](auto texture_width)
      {
         const auto quads_per_row = static_cast<std::size_t>(texture_width / _size);
         return (quads_per_row * quads_per_row >= quad_count);
      }
   );

   if (suitable_texture_width == candidate_texture_widths.end())
   {
      // no square would have held them all, but the atlas does not have to be square any more, so
      // the widest candidate plus however many rows it takes is a perfectly good answer
      _texture_width = candidate_texture_widths.back();
      _log(qApp->tr("no square fits %1 quads; using the widest candidate and growing the rows instead").arg(quad_count));
   }
   else
   {
      _texture_width = *suitable_texture_width;
   }

   // the width has to hold a whole number of quads per row, but the height only has to reach the
   // last row that is actually filled. emitting a square here left the catacombs atlas at
   // 8192x8192 with just 34 of its 128 rows used - 256 MB of texture for 68 MB of content.
   // the runtime derives a quad's column from (index * quad_width) % texture_width and its row by
   // wrapping, so the height never enters the index maths and is free to shrink
   const auto quads_per_row = _texture_width / _size;
   const auto row_count = static_cast<int>((quad_count + quads_per_row - 1) / quads_per_row);
   const auto texture_height = row_count * _size;
   _texture_height = texture_height;

   _log(qApp->tr("picking a %1x%2 texture").arg(_texture_width).arg(texture_height));

   QImage out(_texture_width, texture_height, _image.format());

   // QImage leaves its buffer uninitialised, and the slots after the last quad are never painted,
   // so without this the tail of the atlas is whatever was in memory - different on every run
   out.fill(Qt::transparent);

   QPainter painter(&out);
   painter.setCompositionMode(QPainter::CompositionMode_Source);

   const auto uv_filename = QString("%1_tiles.uv").arg(QFileInfo(_filename).baseName());
   _log(qApp->tr("dumping uvs to: %1").arg(uv_filename));
   std::ofstream uv_file(uv_filename.toStdString());
   auto x = 0;
   auto y = 0;
   auto quad_index = 0;

   for (auto& [_, quad] : _quads)
   {
      const QPoint pos(x, y);
      painter.drawImage(pos, quad._image_data);
      x += _size;

      // go to next row in target texture
      if (x == _texture_width)
      {
         x = 0;
         y += _size;
      }

      if (_update_progress)
      {
         const auto percent = 100 * (static_cast<float>(quad_index) / _quads.size());
         _update_progress(percent);
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
   _log(qApp->tr("dumping texture to: %1").arg(texture_filename));

   if (!out.save(texture_filename))
   {
      _log(qApp->tr("saving failed"));
   }

   if (_update_progress)
   {
      _update_progress(100);
   }

   _log(qApp->tr("done!"));
}
