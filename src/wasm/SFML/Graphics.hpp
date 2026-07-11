#pragma once
#include <string>
#include <vector>

#include <SFML/System.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/DefaultShader.hpp>
#include <SFML/Graphics/DrawableBatch.hpp>
#include <SFML/Graphics/DrawableBatchUtils.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/FontFace.hpp>
#include <SFML/Graphics/Glsl.hpp>
#include <SFML/Graphics/Glyph.hpp>
#include <SFML/Graphics/GraphicsContext.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Shape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/StencilMode.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/TextureAtlas.hpp>
#include <SFML/Graphics/Transform.hpp>
#include <SFML/Graphics/Transformable.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <SFML/Graphics/VertexBuffer.hpp>
#include <SFML/Graphics/View.hpp>

// sf::Drawable was removed in VRSFML; replaced by the DrawableObject concept.
// This compat base satisfies that concept for classes that inherit it.
namespace sf
{
struct Drawable
{
   virtual void draw(RenderTarget& target, RenderStates states) const = 0;
   virtual ~Drawable() = default;
};
}  // namespace sf

// sf::VertexArray was removed in VRSFML. This compat class wraps a vector of
// vertices and a primitive type, providing the same interface as SFML2's
// VertexArray, including draw() so it satisfies the DrawableObject concept.
#include <span>
namespace sf
{
class VertexArray
{
public:
   explicit VertexArray(PrimitiveType type = PrimitiveType::Points, std::size_t initialCount = 0) : _primitive_type(type)
   {
      _vertices.resize(initialCount);
   }

   void append(const Vertex& vertex)
   {
      _vertices.push_back(vertex);
   }
   void clear()
   {
      _vertices.clear();
   }
   void resize(std::size_t count)
   {
      _vertices.resize(count);
   }
   std::size_t getVertexCount() const
   {
      return _vertices.size();
   }
   void setPrimitiveType(PrimitiveType type)
   {
      _primitive_type = type;
   }
   PrimitiveType getPrimitiveType() const
   {
      return _primitive_type;
   }

   Vertex& operator[](std::size_t index)
   {
      return _vertices[index];
   }
   const Vertex& operator[](std::size_t index) const
   {
      return _vertices[index];
   }

   void draw(RenderTarget& target, RenderStates states) const
   {
      if (!_vertices.empty())
      {
         target.draw(std::span<const Vertex>(_vertices.data(), _vertices.size()), _primitive_type, states);
      }
   }

private:
   std::vector<Vertex> _vertices;
   PrimitiveType _primitive_type{};
};
}  // namespace sf
