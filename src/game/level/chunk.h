#ifndef CHUNK_H
#define CHUNK_H

#include <cstdint>

/// \brief chunk grid coordinates used for proximity-based update culling.
/// the idea is that each mechanism or gamenode belongs to a chunk which is a rectangular area within the level.
/// if the chunk doesn't match the player's current chunk, then it won't have to be updated.
/// +-----+-----+-----+-----+
/// | C00 | C10 | C20 | C30 |
/// |     |     |     |     |
/// +-----+-----+-----+-----+
/// | C01 | C11 | C21 | C31 |
/// |     |     |     |     |
/// +-----+-----+-----+-----+
/// | C02 | C12 | C22 | C32 |
/// |     |     |     |     |
/// +-----+-----+-----+-----+
/// | C03 | C13 | C23 | C33 |
/// |     |     |     |     |
/// +-----+-----+-----+-----+

/// \brief identifies a chunk cell in the level chunk grid.
struct Chunk
{
   /// \brief disables default construction because chunk coordinates must be derived from a position.
   Chunk() = delete;
   /// \brief computes chunk indices from a pixel position.
   /// \param x_px horizontal position in pixels.
   /// \param y_px vertical position in pixels.
   Chunk(int32_t x_px, int32_t y_px);
   /// \brief computes chunk indices from a pixel position.
   /// \param x_px horizontal position in pixels.
   /// \param y_px vertical position in pixels.
   Chunk(float x_px, float y_px);

   /// \brief recalculates this chunk from a pixel position.
   /// \param x_px horizontal position in pixels.
   /// \param y_px vertical position in pixels.
   void update(int32_t x_px, int32_t y_px);
   /// \brief compares two chunk coordinates.
   /// \param other chunk to compare against.
   /// \return true when both chunk indices are equal.
   bool operator==(const Chunk& other) const;

   int32_t _x{0};
   int32_t _y{0};
};

#endif  // CHUNK_H
