#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "box2d/box2d.h"
#include <SFML/Graphics.hpp>

namespace Mesh
{

/// \brief obj face vertex indices referencing position, normal, and texture coordinate arrays.
struct Vertex
{
   uint32_t pIndex = 0;
   uint32_t nIndex = 0;
   uint32_t tcIndex = 0;
};

/// \brief scans vertices for near-duplicate positions within a distance threshold.
/// \param verts vertex buffer to inspect.
/// \param count number of vertices in the buffer.
/// \param threshold maximum distance treated as matching positions.
void weldVertices(b2Vec2* verts, int32_t count, float threshold = 0.3f);

/// \brief writes 2d vertices and indexed faces to a wavefront obj file.
/// \param filename destination obj file path.
/// \param vertices point list written as v records.
/// \param faces polygon index lists written as f records.
void writeObj(const std::string& filename, const std::vector<b2Vec2>& vertices, const std::vector<std::vector<uint32_t>>& faces);

/// \brief reads vertex positions and faces from an obj file into 2d geometry buffers.
/// \param filename source obj file path.
/// \param points output vector receiving parsed vertex positions.
/// \param faces output vector receiving parsed face indices.
void readObj(const std::string& filename, std::vector<b2Vec2>& points, std::vector<std::vector<uint32_t>>& faces);

/// \brief rasterizes polygon outlines into an image file for visual debugging.
/// \param points vertex positions used by face indices.
/// \param faces polygon index lists to draw.
/// \param textureSize output image size in pixels.
/// \param imagePath destination path for the generated image file.
/// \param scale scale factor applied to vertex positions before drawing.
void writeVerticesToImage(
   const std::vector<b2Vec2>& points,
   const std::vector<std::vector<uint32_t>>& faces,
   const sf::Vector2i& textureSize,
   const std::filesystem::path& imagePath,
   float scale = 1.0f
);

}  // namespace Mesh
