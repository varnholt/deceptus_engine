#pragma once
#include <string>
#include <vector>

#include <SFML/System.hpp>

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
namespace sf {
    struct Drawable {
        virtual void draw(RenderTarget& target, RenderStates states) const = 0;
        virtual ~Drawable() = default;
    };
}
