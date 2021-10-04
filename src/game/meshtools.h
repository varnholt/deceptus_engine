#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "Box2D/Box2D.h"
#include <SFML/Graphics.hpp>


namespace Mesh
{

struct Vertex {
   uint32_t pIndex = 0;
   uint32_t nIndex = 0;
   uint32_t tcIndex = 0;
};

void weldVertices(b2Vec2* verts, int32_t count, float threshold = 0.3f);

void writeObj(
   const std::string& filename,
   const std::vector<b2Vec2>& vertices,
   const std::vector<std::vector<uint32_t> >& faces
);

void readObj(
   const std::string& filename,
   std::vector<b2Vec2>& points,
   std::vector<std::vector<uint32_t>>& faces
);

void writeVerticesToImage(
   const std::vector<b2Vec2>& points,
   const std::vector<std::vector<uint32_t>>& faces,
   const sf::Vector2i& textureSize,
   const std::filesystem::path& imagePath
);

}
