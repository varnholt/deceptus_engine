#include "ingamemenulabels.h"

#include "framework/tools/sfmlcompat.h"
#include "game/config/gameconfiguration.h"
#include "game/ui/menulabel.h"

#include <algorithm>
#include <cmath>

namespace
{

constexpr auto character_size = 12u;

//! horizontal room the pill keeps on either side of the word it holds
constexpr auto pill_padding_px = 12;

//! distance between the centers of two tabs in the original artwork
constexpr auto tab_spacing_px = 76;

//! columns taken from either end of the pill; everything between them is one stretched column
constexpr auto pill_cap_width_px = 12;

//! gap between a button icon and the word next to it
constexpr auto hint_icon_gap_px = 6;

//! gap between two button hints in the footer
constexpr auto hint_spacing_px = 24;

const sf::Color color_tab_selected{212, 219, 235};
const sf::Color color_tab_normal{83, 100, 171};
const sf::Color color_hint_plain{160, 173, 203};
const sf::Color color_hint_pressed{217, 222, 234};

/// \brief returns the width of a translated label, rounded up to whole pixels.
/// \param source_text english source text.
/// \return width in pixels, at least one.
int32_t labelWidth(const std::string& source_text)
{
   return std::max(1, static_cast<int32_t>(std::ceil(MenuLabel::measure(source_text, character_size))));
}

/// \brief returns the x a row of the given width starts at to sit centered on the view.
/// \param width_px width of the row.
/// \return x position in whole pixels.
int32_t centeredRowX(int32_t width_px)
{
   const auto view_width = static_cast<int32_t>(GameConfiguration::getInstance()._view_width);
   return (view_width - width_px) / 2;
}

/// \brief redraws one footer hint so its word comes from the translation table.
/// \param layer layer to redraw; nothing happens when it is not there.
/// \param icon_width_px columns of the layer image the icon occupies.
/// \param source_text english source text of the word.
/// \param color color of the word.
/// \param item_width_px width of the new layer image.
/// \param x_px x the layer is moved to.
void updateHintLabel(Layer* layer, int32_t icon_width_px, const std::string& source_text, const sf::Color& color, int32_t item_width_px, int32_t x_px)
{
   if (layer == nullptr || !layer->_texture || !layer->_sprite)
   {
      return;
   }

   const auto layer_size = layer->_texture->getSize();
   const auto height_px = static_cast<int32_t>(layer_size.y);
   const auto position = sfcompat::getPosition(*layer->_sprite);

   MenuLabel::compose(
      *layer,
      {item_width_px, height_px},
      {MenuLabel::Piece{._source = sf::IntRect{{0, 0}, {icon_width_px, height_px}}, ._target = sf::Vector2i{0, 0}}},
      {MenuLabel::Label{
         ._text = source_text,
         ._box = sf::IntRect{{icon_width_px + hint_icon_gap_px, 0}, {item_width_px - icon_width_px - hint_icon_gap_px, height_px}},
         ._align = MenuLabel::Align::Left,
         ._character_size = character_size,
         ._color = color
      }}
   );

   sfcompat::setPosition(*layer->_sprite, {static_cast<float>(x_px), position.y});
}

}  // namespace

void InGameMenuLabels::updateHeaderLabels(Layer& header_layer, Tab selected, int32_t pill_left_px, int32_t pill_width_px)
{
   if (!header_layer._texture || !header_layer._sprite)
   {
      return;
   }

   const std::string tab_names[] = {"Map", "Inventory", "Archives"};

   const auto layer_size = header_layer._texture->getSize();
   const auto height_px = static_cast<int32_t>(layer_size.y);

   auto widest_label_px = 0;
   for (const auto& tab_name : tab_names)
   {
      widest_label_px = std::max(widest_label_px, labelWidth(tab_name));
   }

   const auto new_pill_width_px = std::max(pill_width_px, widest_label_px + 2 * pill_padding_px);
   const auto spacing_px = std::max(tab_spacing_px, new_pill_width_px + 6);
   const auto image_width_px = new_pill_width_px + 2 * spacing_px;

   const auto selected_index = static_cast<int32_t>(selected);
   const auto pill_x_px = selected_index * spacing_px;

   const std::vector<MenuLabel::Piece> pieces{
      MenuLabel::Piece{
         ._source = sf::IntRect{{pill_left_px, 0}, {pill_cap_width_px, height_px}},  //
         ._target = sf::Vector2i{pill_x_px, 0}
      },
      MenuLabel::Piece{
         ._source = sf::IntRect{{pill_left_px + pill_cap_width_px, 0}, {1, height_px}},
         ._target = sf::Vector2i{pill_x_px + pill_cap_width_px, 0},
         ._size = sf::Vector2i{new_pill_width_px - 2 * pill_cap_width_px, height_px}
      },
      MenuLabel::Piece{
         ._source = sf::IntRect{{pill_left_px + pill_width_px - pill_cap_width_px, 0}, {pill_cap_width_px, height_px}},
         ._target = sf::Vector2i{pill_x_px + new_pill_width_px - pill_cap_width_px, 0}
      },
   };

   std::vector<MenuLabel::Label> labels;
   for (auto index = 0; index < 3; index++)
   {
      labels.push_back(MenuLabel::Label{
         ._text = tab_names[index],
         ._box = sf::IntRect{{index * spacing_px, 0}, {new_pill_width_px, height_px}},
         ._align = MenuLabel::Align::Centered,
         ._character_size = character_size,
         ._color = (index == selected_index) ? color_tab_selected : color_tab_normal
      });
   }

   const auto position = sfcompat::getPosition(*header_layer._sprite);
   MenuLabel::compose(header_layer, {image_width_px, height_px}, pieces, labels);
   sfcompat::setPosition(*header_layer._sprite, {static_cast<float>(centeredRowX(image_width_px)), position.y});
}

void InGameMenuLabels::updateFooterLabels(const std::vector<FooterHint>& hints)
{
   std::vector<int32_t> item_widths;
   item_widths.reserve(hints.size());

   auto row_width_px = 0;
   for (const auto& hint : hints)
   {
      const auto item_width_px = hint._icon_width_px + hint_icon_gap_px + labelWidth(hint._text);
      item_widths.push_back(item_width_px);
      row_width_px += item_width_px;
   }

   if (hints.size() > 1)
   {
      row_width_px += static_cast<int32_t>(hints.size() - 1) * hint_spacing_px;
   }

   auto x_px = centeredRowX(row_width_px);
   for (auto index = size_t{0}; index < hints.size(); index++)
   {
      const auto& hint = hints[index];
      updateHintLabel(hint._layer_plain, hint._icon_width_px, hint._text, color_hint_plain, item_widths[index], x_px);
      updateHintLabel(hint._layer_pressed, hint._icon_width_px, hint._text, color_hint_pressed, item_widths[index], x_px);
      x_px += item_widths[index] + hint_spacing_px;
   }
}
