#ifndef QUAD_H
#define QUAD_H

#include <cstdint>
#include <QImage>

struct Quad
{
  int32_t x_;
  int32_t y_;
  int32_t w_;
  int32_t h_;
  QImage data_;
};

#endif // QUAD_H
