#include "chunk.h"

#include "constants.h"

Chunk::Chunk(int32_t x_px, int32_t y_px)
{
   update(x_px, y_px);
}

Chunk::Chunk(float x_px, float y_px)
{
   update(static_cast<int32_t>(x_px), static_cast<int32_t>(y_px));
}

void Chunk::update(int32_t x_px, int32_t y_px)
{
   _x = x_px >> CHUNK_SHIFT_X;
   _y = y_px >> CHUNK_SHIFT_Y;
}

bool Chunk::operator==(const Chunk& other) const
{
   return _x == other._x && _y == other._y;
}
