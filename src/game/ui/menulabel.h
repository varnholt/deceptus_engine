#pragma once

#include "framework/image/layer.h"

#include <SFML/Graphics.hpp>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

/// \brief draws the words of a psd layer from the translation table.
///
/// the menus are photoshop documents, and a label such as 'Inventory' or 'Navigate' cannot be
/// translated while it is artwork. a layer is composed here instead: the parts of it that are not
/// words -- an icon, a flourish, the caps of a highlight pill -- are kept, and the label is drawn
/// next to them at whatever width the translation needs.
///
/// the result goes back into the layer's own texture, so everything that already moves, fades or
/// hides that layer keeps working without knowing that a label is text.
namespace MenuLabel
{

/// \brief a rectangular piece of the original layer image that is kept.
struct Piece
{
   sf::IntRect _source;                //!< region of the original layer image
   sf::Vector2i _target;               //!< where that region goes in the new image
   std::optional<sf::Vector2i> _size;  //!< size to scale the region to; unset keeps the source size
};

/// \brief horizontal placement of a label inside its box.
enum class Align
{
   Left,
   Centered,
   Right
};

/// \brief a translated word drawn into the new image.
struct Label
{
   std::string _text;             //!< english source text, looked up through tr()
   sf::IntRect _box;              //!< region of the new image the word is placed in
   Align _align{Align::Centered};
   uint32_t _character_size{12};
   sf::Color _color{255, 255, 255};
};

/// \brief returns how wide a translated label renders.
/// \param source_text english source text, looked up through tr().
/// \param character_size character size the label will be rendered at.
/// \return width in pixels.
[[nodiscard]] float measure(const std::string& source_text, uint32_t character_size);

/// \brief composes a layer's image out of pieces of itself plus translated labels.
///
/// the layer keeps the position its sprite had, so a caller that only replaces words inside the
/// original bounds has nothing else to do. a caller that needs more room for a longer translation
/// passes a bigger size and moves the layer afterwards.
///
/// \param layer layer to compose; its texture and sprite are replaced.
/// \param size size of the new image; pass the layer's own size to keep it.
/// \param pieces parts of the original image that are kept, drawn in the given order.
/// \param labels words drawn on top of them.
void compose(Layer& layer, const sf::Vector2i& size, const std::vector<Piece>& pieces, const std::vector<Label>& labels);

/// \brief convenience overload that keeps the layer's current size.
/// \param layer layer to compose; its texture and sprite are replaced.
/// \param pieces parts of the original image that are kept, drawn in the given order.
/// \param labels words drawn on top of them.
void compose(Layer& layer, const std::vector<Piece>& pieces, const std::vector<Label>& labels);

}  // namespace MenuLabel
