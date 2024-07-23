#pragma once

#include <QImage>
#include <cstdint>

#include <vector>

struct Rect
{
   int32_t _x{0};
   int32_t _y{0};
   int32_t _w{0};
   int32_t _h{0};
};

struct Quad
{
   std::vector<Rect> _rects;
   QImage _image_data;
};
