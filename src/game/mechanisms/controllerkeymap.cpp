#include "controllerkeymap.h"

#include <algorithm>

#include "framework/joystick/gamecontroller.h"
#include "game/controller/gamecontrollerintegration.h"

// SDL
#ifdef TARGET_OS_MAC
#ifdef MAC_OS_X_VERSION_MIN_REQUIRED
#undef MAC_OS_X_VERSION_MIN_REQUIRED
#endif
#define MAC_OS_X_VERSION_MIN_REQUIRED MAC_OS_X_VERSION_10_9
#endif
#include <SDL3/SDL.h>

namespace
{

// the icon atlas has no keycap for every key the player may bind, so keys such as the function row,
// the numpad or the navigation block resolve to an empty id and are reported by the caller
const std::map<sf::Keyboard::Key, std::string>& getKeyboardKeyToIconMap()
{
   static const std::map<sf::Keyboard::Key, std::string> keyboard_key_to_icon = {
      {sf::Keyboard::Key::A, "key_a"},
      {sf::Keyboard::Key::B, "key_b"},
      {sf::Keyboard::Key::C, "key_c"},
      {sf::Keyboard::Key::D, "key_d"},
      {sf::Keyboard::Key::E, "key_e"},
      {sf::Keyboard::Key::F, "key_f"},
      {sf::Keyboard::Key::G, "key_g"},
      {sf::Keyboard::Key::H, "key_h"},
      {sf::Keyboard::Key::I, "key_i"},
      {sf::Keyboard::Key::J, "key_j"},
      {sf::Keyboard::Key::K, "key_k"},
      {sf::Keyboard::Key::L, "key_l"},
      {sf::Keyboard::Key::M, "key_m"},
      {sf::Keyboard::Key::N, "key_n"},
      {sf::Keyboard::Key::O, "key_o"},
      {sf::Keyboard::Key::P, "key_p"},
      {sf::Keyboard::Key::Q, "key_q"},
      {sf::Keyboard::Key::R, "key_r"},
      {sf::Keyboard::Key::S, "key_s"},
      {sf::Keyboard::Key::T, "key_t"},
      {sf::Keyboard::Key::U, "key_u"},
      {sf::Keyboard::Key::V, "key_v"},
      {sf::Keyboard::Key::W, "key_w"},
      {sf::Keyboard::Key::X, "key_x"},
      {sf::Keyboard::Key::Y, "key_y"},
      {sf::Keyboard::Key::Z, "key_z"},
      {sf::Keyboard::Key::Num0, "key_0"},
      {sf::Keyboard::Key::Num1, "key_1"},
      {sf::Keyboard::Key::Num2, "key_2"},
      {sf::Keyboard::Key::Num3, "key_3"},
      {sf::Keyboard::Key::Num4, "key_4"},
      {sf::Keyboard::Key::Num5, "key_5"},
      {sf::Keyboard::Key::Num6, "key_6"},
      {sf::Keyboard::Key::Num7, "key_7"},
      {sf::Keyboard::Key::Num8, "key_8"},
      {sf::Keyboard::Key::Num9, "key_9"},
      {sf::Keyboard::Key::Escape, "key_esc"},
      {sf::Keyboard::Key::LControl, "key_ctrl"},
      {sf::Keyboard::Key::RControl, "key_ctrl"},
      {sf::Keyboard::Key::LShift, "key_shift"},
      {sf::Keyboard::Key::RShift, "key_shift"},
      {sf::Keyboard::Key::LAlt, "key_alt"},
      {sf::Keyboard::Key::RAlt, "key_alt"},
      {sf::Keyboard::Key::LSystem, "key_win"},
      {sf::Keyboard::Key::RSystem, "key_win"},
      {sf::Keyboard::Key::Menu, "key_list"},
      {sf::Keyboard::Key::LBracket, "key_bracket_l"},
      {sf::Keyboard::Key::RBracket, "key_bracket_r"},
      {sf::Keyboard::Key::Semicolon, "key_semicolon"},
      {sf::Keyboard::Key::Comma, "key_comma"},
      {sf::Keyboard::Key::Period, "key_period"},
      {sf::Keyboard::Key::Apostrophe, "key_apostrophe"},
      {sf::Keyboard::Key::Slash, "key_question"},
      {sf::Keyboard::Key::Backslash, "key_backslash"},
      {sf::Keyboard::Key::Equal, "key_equals"},
      {sf::Keyboard::Key::Hyphen, "key_minus"},
      {sf::Keyboard::Key::Space, "key_space"},
      {sf::Keyboard::Key::Enter, "key_return"},
      {sf::Keyboard::Key::Backspace, "key_backspace"},
      {sf::Keyboard::Key::Tab, "key_tab"},
      {sf::Keyboard::Key::Left, "key_cursor_l"},
      {sf::Keyboard::Key::Right, "key_cursor_r"},
      {sf::Keyboard::Key::Up, "key_cursor_u"},
      {sf::Keyboard::Key::Down, "key_cursor_d"},
   };
   return keyboard_key_to_icon;
}

const std::map<int32_t, std::string>& getControllerButtonToIconMap()
{
   static const std::map<int32_t, std::string> controller_button_to_icon = {
      {SDL_GAMEPAD_BUTTON_SOUTH, "bt_a"},
      {SDL_GAMEPAD_BUTTON_EAST, "bt_b"},
      {SDL_GAMEPAD_BUTTON_WEST, "bt_x"},
      {SDL_GAMEPAD_BUTTON_NORTH, "bt_y"},
      // the two ids read backwards: bt_list carries the three line glyph, which every pad in the atlas
      // uses for its start button, and bt_menu carries the two rectangle glyph, which is the back
      // button. the ids are what the tmx files already spell out, so the artwork decides the mapping
      {SDL_GAMEPAD_BUTTON_BACK, "bt_menu"},
      {SDL_GAMEPAD_BUTTON_START, "bt_list"},
      {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, "bt_lb"},
      {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, "bt_rb"},
      {SDL_GAMEPAD_BUTTON_DPAD_UP, "dpad_u"},
      {SDL_GAMEPAD_BUTTON_DPAD_DOWN, "dpad_d"},
      {SDL_GAMEPAD_BUTTON_DPAD_LEFT, "dpad_l"},
      {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, "dpad_r"},
   };
   return controller_button_to_icon;
}

constexpr auto key_map_column_count = 16;

/// \brief looks one icon id up in a brand table.
/// \param icon_id logical icon name to find.
/// \param cells brand table to search.
/// \return {column, row} of the matching cell, or nothing when the table has no such id.
template <std::size_t cell_count>
std::optional<std::pair<int32_t, int32_t>> findInBrandTable(
   const std::string& icon_id,
   const std::array<ControllerKeyMap::IconCell, cell_count>& cells
)
{
   const auto matching_cell = std::find_if(
      cells.cbegin(), cells.cend(), [&icon_id](const auto& cell) { return cell._icon_id == icon_id; }
   );

   if (matching_cell == cells.cend())
   {
      return std::nullopt;
   }

   return std::make_pair(matching_cell->_column, matching_cell->_row);
}

/// \brief looks one icon id up in the large blocks key_map covers.
/// \param icon_id logical icon name to find.
/// \return {column, row} of the matching cell, or nothing when key_map has no such id.
std::optional<std::pair<int32_t, int32_t>> findInKeyMap(const std::string& icon_id)
{
   const auto matching_entry = std::find(ControllerKeyMap::key_map.cbegin(), ControllerKeyMap::key_map.cend(), icon_id);

   if (matching_entry == ControllerKeyMap::key_map.cend())
   {
      return std::nullopt;
   }

   const auto index = static_cast<int32_t>(std::distance(ControllerKeyMap::key_map.cbegin(), matching_entry));

   return std::make_pair(index % key_map_column_count, index / key_map_column_count);
}

/// \brief looks one keyboard icon id up in the small keyboard block.
/// \param icon_id logical icon name to find.
/// \return {column, row} of the matching cell, or nothing when the small block has no such keycap.
std::optional<std::pair<int32_t, int32_t>> findInSmallKeyboardBlock(const std::string& icon_id)
{
   const auto excluded_id = std::find(
      ControllerKeyMap::keyboard_ids_without_small_cell.cbegin(), ControllerKeyMap::keyboard_ids_without_small_cell.cend(), icon_id
   );

   if (excluded_id != ControllerKeyMap::keyboard_ids_without_small_cell.cend())
   {
      return std::nullopt;
   }

   const auto large_position = findInKeyMap(icon_id);
   if (!large_position.has_value())
   {
      return std::nullopt;
   }

   // only the five keyboard rows of key_map have a small counterpart, the pad rows below them do not
   constexpr auto keyboard_row_count = 5;
   if (large_position->second >= keyboard_row_count)
   {
      return std::nullopt;
   }

   return std::make_pair(large_position->first, large_position->second + ControllerKeyMap::keyboard_small_row_offset);
}

/// \brief looks one icon id up in the tables of a single brand.
/// \param icon_id logical icon name to find.
/// \param brand controller family whose tables are searched.
/// \param size size variant to look for; a small request falls back to the brand's large table.
/// \return {column, row} of the matching cell, or nothing when the brand has no such icon.
std::optional<std::pair<int32_t, int32_t>>
findInBrand(const std::string& icon_id, ControllerKeyMap::IconBrand brand, ControllerKeyMap::IconSize size)
{
   const auto want_small = (size == ControllerKeyMap::IconSize::Small);

   switch (brand)
   {
      case ControllerKeyMap::IconBrand::Xbox:
      {
         // the large xbox artwork is row 5 of key_map, so there is no large table to fall back to here
         if (want_small)
         {
            return findInBrandTable(icon_id, ControllerKeyMap::xbox_small_map);
         }
         return std::nullopt;
      }
      case ControllerKeyMap::IconBrand::PlayStation:
      {
         if (want_small)
         {
            const auto small_position = findInBrandTable(icon_id, ControllerKeyMap::playstation_small_map);
            if (small_position.has_value())
            {
               return small_position;
            }
         }
         return findInBrandTable(icon_id, ControllerKeyMap::playstation_large_map);
      }
      case ControllerKeyMap::IconBrand::Switch:
      {
         if (want_small)
         {
            const auto small_position = findInBrandTable(icon_id, ControllerKeyMap::switch_small_map);
            if (small_position.has_value())
            {
               return small_position;
            }
         }
         return findInBrandTable(icon_id, ControllerKeyMap::switch_large_map);
      }
      case ControllerKeyMap::IconBrand::SteamDeck:
      {
         if (want_small)
         {
            const auto small_position = findInBrandTable(icon_id, ControllerKeyMap::steam_deck_small_map);
            if (small_position.has_value())
            {
               return small_position;
            }
         }
         return findInBrandTable(icon_id, ControllerKeyMap::steam_deck_large_map);
      }
   }

   return std::nullopt;
}

}  // namespace

std::optional<std::pair<int32_t, int32_t>>
ControllerKeyMap::getArrayPosition(const std::string& icon_id, IconBrand brand, IconSize size)
{
   // key_map is padded with empty entries, so an empty id would resolve to one of them
   if (icon_id.empty())
   {
      return std::nullopt;
   }

   const auto brand_position = findInBrand(icon_id, brand, size);
   if (brand_position.has_value())
   {
      return brand_position;
   }

   if (size == IconSize::Small)
   {
      const auto small_keyboard_position = findInSmallKeyboardBlock(icon_id);
      if (small_keyboard_position.has_value())
      {
         return small_keyboard_position;
      }
   }

   return findInKeyMap(icon_id);
}

std::pair<std::string, std::string> ControllerKeyMap::retrieveMappedKey(const std::string& key)
{
   const auto key_it = key_controller_map.find(key);

   // key has no controller buddy, just use same identifier for both keyboard and controller
   if (key_it == key_controller_map.cend())
   {
      return {key, key};
   }

   // return keyboard <-> controller tuple
   return {key, key_it->second};
}

std::string ControllerKeyMap::iconNameForKeyboardKey(sf::Keyboard::Key key)
{
   const auto& keyboard_key_to_icon = getKeyboardKeyToIconMap();
   const auto icon_it = keyboard_key_to_icon.find(key);

   if (icon_it == keyboard_key_to_icon.cend())
   {
      return {};
   }

   return icon_it->second;
}

std::string ControllerKeyMap::iconNameForControllerButton(int32_t sdl_button)
{
   const auto& controller_button_to_icon = getControllerButtonToIconMap();
   const auto icon_it = controller_button_to_icon.find(sdl_button);

   if (icon_it == controller_button_to_icon.cend())
   {
      return {};
   }

   return icon_it->second;
}

ControllerKeyMap::IconBrand ControllerKeyMap::brandForConnectedController()
{
#ifdef __SWITCH__
   // the console build always names the console's own buttons, whatever pad happens to be paired
   return IconBrand::Switch;
#else
   const auto& integration = GameControllerIntegration::getInstance();

   // getController throws for an id that is not in the map, and the selected id is not cleared when a
   // pad is unplugged, so the id is taken from the live list rather than from the selection
   const auto controller_ids = integration.getControllerIds();
   if (controller_ids.empty())
   {
      return default_icon_brand;
   }

   const auto& controller = integration.getController(controller_ids.front());
   if (!controller)
   {
      return default_icon_brand;
   }

   // valve has no SDL_GamepadType of its own, so the deck's built-in controls are recognized by their
   // usb ids. this misses the case where steam input interposes its own virtual pad, which reports as
   // an xbox device -- the xbox artwork is the right fallback for that anyway
   constexpr uint16_t valve_vendor_id = 0x28de;
   constexpr uint16_t steam_deck_product_id = 0x1205;
   if (controller->getVendorId() == valve_vendor_id && controller->getProductId() == steam_deck_product_id)
   {
      return IconBrand::SteamDeck;
   }

   switch (controller->getGamepadType())
   {
      case SDL_GAMEPAD_TYPE_PS3:
      case SDL_GAMEPAD_TYPE_PS4:
      case SDL_GAMEPAD_TYPE_PS5:
      {
         return IconBrand::PlayStation;
      }
      case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
      case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
      case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
      case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
      {
         return IconBrand::Switch;
      }
      default:
      {
         break;
      }
   }

   return default_icon_brand;
#endif
}
