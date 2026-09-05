#pragma once

#include "framework/image/layer.h"

#include <SFML/Graphics.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

/// \brief the labels the map, inventory and archives pages have in common.
///
/// all three carry the same tab strip at the top and the same kind of button hints along the
/// bottom, and in the photoshop documents both were artwork rather than text. the functions here
/// redraw them from the translation table and lay them out again, since a translated word is rarely
/// as wide as the english one it replaces.
namespace InGameMenuLabels
{

/// \brief the pages the tab strip switches between.
enum class Tab
{
   Map,
   Inventory,
   Archives
};

/// \brief a button hint in the footer: an icon with a word next to it.
struct FooterHint
{
   std::shared_ptr<Layer> _layer_plain;    //!< the '_0' layer, shown while the button is not pressed
   std::shared_ptr<Layer> _layer_pressed;  //!< the '_1' layer, shown while it is
   int32_t _icon_width_px{0};       //!< columns of the layer image the icon occupies
   std::string _text;               //!< english source text of the word
};

/// \brief redraws the tab strip with translated tab names and centers it again.
///
/// the strip is one image holding all three names and, behind the name of the page it belongs to, a
/// magenta pill. the pill is rebuilt from its own left and right cap with the middle stretched, so
/// it grows with whatever word goes into it.
///
/// \param header_layer the 'header' layer of the page.
/// \param selected tab whose name carries the pill.
/// \param pill_left_px first column of the pill inside the original layer image.
/// \param pill_width_px width of the pill in the original layer image.
void updateHeaderLabels(Layer& header_layer, Tab selected, int32_t pill_left_px, int32_t pill_width_px);

/// \brief redraws a row of footer hints and lays them out again, centered on the view.
/// \param hints hints in the order they appear, left to right.
void updateFooterLabels(const std::vector<FooterHint>& hints);

}  // namespace InGameMenuLabels
