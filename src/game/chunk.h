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
   int32_t _x = 0;
   int32_t _y = 0;

   static constexpr int32_t _width = 512;
   static constexpr int32_t _height = 512;
};

#endif // CHUNK_H
