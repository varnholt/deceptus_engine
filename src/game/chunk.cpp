#include "chunk.h"

#include "constants.h"

Chunk::Chunk(int32_t x_px, int32_t y_px) : _x(x_px >> CHUNK_SHIFT_X), _y(y_px >> CHUNK_SHIFT_Y)
{
}

Chunk::Chunk(float x_px, float y_px) : _x(static_cast<int32_t>(x_px) >> CHUNK_SHIFT_X), _y(static_cast<int32_t>(y_px) >> CHUNK_SHIFT_Y)
{
}

void Chunk::update(int32_t x_px, int32_t y_px)
{
   _x = x_px >> CHUNK_SHIFT_X;
   _y = y_px >> CHUNK_SHIFT_Y;
}

bool Chunk::operator==(const Chunk& other)
{
   return _x == other._x && _y == other._y;
}
