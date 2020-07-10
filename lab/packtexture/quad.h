#pragma once

#include <cstdint>
#include <QImage>

struct Quad
{
   int32_t mX;
   int32_t mY;
   int32_t mW;
   int32_t mH;
   QImage mData;
};

