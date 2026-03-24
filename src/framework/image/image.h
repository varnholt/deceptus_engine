#pragma once

#include <cstdint>
#include <string>
#include <vector>

///
/// \brief Stores ARGB image data and provides pixel-processing utilities.
///
class Image
{
public:
   Image() = default;

   ///
   /// \brief Constructs an image and loads it from file.
   /// \param filename Source file path.
   ///
   Image(const std::string& filename);

   ///
   /// \brief Copies image dimensions and pixel data from another image.
   /// \param image Source image.
   ///
   Image(const Image& image);

   ///
   /// \brief Assigns dimensions and pixel data from another image.
   /// \param image Source image.
   /// \return Assigned image reference.
   ///
   const Image& operator=(const Image& image);

   ///
   /// \brief Resizes image storage and clears all pixels to transparent black.
   /// \param x Image width in pixels.
   /// \param y Image height in pixels.
   ///
   void init(int32_t x, int32_t y);

   ///
   /// \brief Fills the entire image with one ARGB color.
   /// \param argb Fill color in ARGB layout.
   ///
   void clear(uint32_t argb);

   ///
   /// \brief Writes the image to a TGA file.
   /// \param filename Output file path.
   ///
   void save(const std::string& filename) const;

   ///
   /// \brief Returns image width in pixels.
   /// \return Width in pixels.
   ///
   int32_t getWidth() const;

   ///
   /// \brief Returns image height in pixels.
   /// \return Height in pixels.
   ///
   int32_t getHeight() const;

   ///
   /// \brief Returns a mutable pointer to the first pixel of scanline `y`.
   /// \param y Scanline index.
   /// \return Pointer to the scanline start.
   ///
   uint32_t* getScanline(int32_t y) const;

   ///
   /// \brief Returns the backing ARGB pixel buffer.
   /// \return Pixel buffer.
   ///
   const std::vector<uint32_t>& getData() const;

   ///
   /// \brief Builds a half-resolution image by averaging 2x2 source pixels.
   /// \return Downsampled image.
   ///
   Image downsample() const;

   ///
   /// \brief Bilinearly samples `image` into this image's dimensions.
   /// \param image Source image to scale from.
   ///
   void scaled(const Image& image) const;

   ///
   /// \brief Loads image data from file.
   /// \param filename Input file path.
   ///
   /// currently this function is a placeholder and does not populate pixels.
   ///
   void load(const std::string& filename);

   ///
   /// \brief Copies `source` into this image at `(x, y)`.
   /// \param x Destination x coordinate.
   /// \param y Destination y coordinate.
   /// \param source Source image.
   /// \param clamp Optional edge replication count for right/bottom borders.
   ///
   void copy(int32_t x, int32_t y, const Image& source, int32_t clamp = 0);

   ///
   /// \brief Multiplies rgb channels by alpha for each pixel.
   ///
   void premultiplyAlpha();

   ///
   /// \brief Replaces each pixel with the per-channel minimum against `image`.
   /// \param image Comparison image.
   ///
   void minimum(const Image& image);

   ///
   /// \brief Samples a pixel using normalized uv coordinates.
   /// \param u Horizontal coordinate in [0, 1].
   /// \param v Vertical coordinate in [0, 1].
   /// \return Sampled ARGB pixel.
   ///
   uint32_t getPixel(float u, float v) const;

   ///
   /// \brief Builds a normal map from image luminance and returns a new buffer.
   /// \param z Strength term used for the normal z component.
   /// \return Heap-allocated normal-map buffer.
   ///
   uint32_t* buildNormalMap(int32_t z);

   ///
   /// \brief Builds a delta map from neighboring pixel differences.
   /// \return Heap-allocated delta-map buffer.
   ///
   uint32_t* buildDeltaMap();

   ///
   /// \brief Returns the source path recorded for this image.
   /// \return Stored path string.
   ///
   const std::string& path() const;

   ///
   /// \brief Returns the source filename recorded for this image.
   /// \return Stored filename string.
   ///
   const std::string& filename() const;

private:
   std::vector<uint32_t> _data;
   int32_t _width = 0;
   int32_t _height = 0;
   std::string _path;
   std::string _filename;
};
