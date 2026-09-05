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

/// \brief one rectangle of the layer's current image that the rebuilt image keeps.
///
/// compose() draws these and nothing else before it draws the labels, so whatever no region covers
/// is gone from the result. that is how the english word baked into the artwork disappears: the
/// regions name the icon, the flourish and the rounded ends of a plate, and the word between them
/// is simply never copied over.
struct KeptRegion
{
   sf::IntRect _source;                //!< rectangle of the layer's current image
   sf::Vector2i _target;               //!< where that rectangle goes in the new image
   std::optional<sf::Vector2i> _size;  //!< size to scale it to; unset keeps the size it had
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

/// \brief returns how wide a translated label renders, in whole pixels.
///
/// rounded up, and never zero: a caller sizes an image with this, and an image of no width is not a
/// thing that can be created.
///
/// \param source_text english source text, looked up through tr().
/// \param character_size character size the label will be rendered at.
/// \return width in pixels, at least one.
[[nodiscard]] int32_t measureWidth(const std::string& source_text, uint32_t character_size);

/// \brief builds the three regions that redraw a plate at a width it was not drawn at.
///
/// a plate with rounded ends -- the highlight pill of the tab strip, the plate behind a filter name
/// -- cannot simply be scaled, or its corners scale with it. its two ends are kept as they are and
/// the single column between them is stretched to fill whatever is left, so the plate grows with
/// the word it holds while its corners stay the shape the artist drew.
///
/// \param source rectangle of the plate in the original layer image.
/// \param target_x_px x the plate is redrawn at; it keeps the top of the image.
/// \param width_px width to redraw the plate at.
/// \param cap_width_px columns taken from either end of it.
/// \return the three regions, left to right.
[[nodiscard]] std::vector<KeptRegion> stretchedPlate(const sf::IntRect& source, int32_t target_x_px, int32_t width_px, int32_t cap_width_px);

/// \brief composes a layer's image out of rectangles of itself plus translated labels.
///
/// the layer keeps the position its sprite had, so a caller that only replaces words inside the
/// original bounds has nothing else to do. a caller that needs more room for a longer translation
/// passes a bigger size and moves the layer afterwards.
///
/// \param layer layer to compose; its texture and sprite are replaced.
/// \param size size of the new image; pass the layer's own size to keep it.
/// \param kept_regions rectangles of the current image that are kept, drawn in the given order.
/// \param labels words drawn on top of them.
void compose(Layer& layer, const sf::Vector2i& size, const std::vector<KeptRegion>& kept_regions, const std::vector<Label>& labels);

/// \brief convenience overload that keeps the layer's current size.
/// \param layer layer to compose; its texture and sprite are replaced.
/// \param kept_regions rectangles of the current image that are kept, drawn in the given order.
/// \param labels words drawn on top of them.
void compose(Layer& layer, const std::vector<KeptRegion>& kept_regions, const std::vector<Label>& labels);

}  // namespace MenuLabel
