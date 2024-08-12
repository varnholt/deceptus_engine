#ifndef CHUNK_H
#define CHUNK_H

#include <cstdint>

/*! \brief A Chunk datastructure
 *
 * The idea is that each mechanism or gamenode belongs to a chunk which is a rectangular area within the level.
 * If the chunk doesn't match the player's current chunk, then it won't have to be updated.
 *
 * +-----+-----+-----+-----+
 * | C00 | C10 | C20 | C30 |
 * |     |     |     |     |
 * +-----+-----+-----+-----+
 * | C01 | C11 | C21 | C31 |
 * |     |     |     |     |
 * +-----+-----+-----+-----+
 * | C02 | C12 | C22 | C32 |
 * |     |     |     |     |
 * +-----+-----+-----+-----+
 * | C03 | C13 | C23 | C33 |
 * |     |     |     |     |
 * +-----+-----+-----+-----+
 */

struct Chunk
{
   Chunk() = default;
   Chunk(int32_t x_px, int32_t y_px);
   Chunk(float x_px, float y_px);

   void update(int32_t x_px, int32_t y_px);
   bool operator==(const Chunk& other) const;

   int32_t _x{0};
   int32_t _y{0};
};

#endif  // CHUNK_H
