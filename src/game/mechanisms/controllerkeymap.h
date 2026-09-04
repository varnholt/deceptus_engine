#ifndef CONTROLLERKEYMAP_H
#define CONTROLLERKEYMAP_H

#include <SFML/Window/Keyboard.hpp>

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace ControllerKeyMap
{

// clang-format off
static const std::array<std::string, 16 * 9> key_map{
   "key_esc",   "key_1",   "key_2",   "key_3",     "key_4",    "key_5",   "key_6",       "key_7",       "key_8",       "key_9",             "key_minus",     "key_equals",     "key_backspace", "",     "",       "",
   "key_tab",   "key_q",   "key_w",   "key_e",     "key_r",    "key_t",   "key_y",       "key_u",       "key_i",       "key_o",             "key_p",         "key_bracket_l",  "key_bracket_r", "",     "",       "",
   "key_caps",  "key_a",   "key_s",   "key_d",     "key_f",    "key_g",   "key_h",       "key_j",       "key_k",       "key_l",             "key_semicolon", "key_apostrophe", "key_return",    "",     "",       "",
   "key_shift", "key_0",   "key_z",   "key_x",     "key_c",    "key_v",   "key_b",       "key_n",       "key_m",       "key_comma",         "key_period",    "key_question",   "key_backslash", "",     "",       "",
   "key_ctrl",  "key_win", "key_alt", "key_space", "key_list",  "",        "",           "",            "",            "key_cursor_l",      "key_cursor_u",  "key_cursor_d",   "key_cursor_r",  "",     "",       "",
   "bt_a",      "bt_b",    "bt_x",    "bt_y",      "bt_list",  "bt_menu",  "bt_rt",      "bt_lt",       "bt_lb",       "bt_rb",              "",              "",               "",             "",     "",       "",
   "dpad_u",    "dpad_d",  "dpad_l",  "dpad_r",    "bt_u",     "bt_d",     "bt_l",       "bt_r",        "bt_1",        "bt_2",               "bt_3",          "bt_4",           "bt_5",        "bt_6",  "bt_7",  "bt_8",
   "bt_r_u",    "bt_r_d",  "bt_r_l",  "bt_r_r",    "bt_r_u_d", "bt_r_l_r", "dpad_empty", "bt_0",        "bt_9",        "bt_10",              "bt_11",         "bt_12",          "bt_13",       "bt_14", "bt_15", "bt_16",
   "bt_l_u",    "bt_l_d",  "bt_l_l",  "bt_l_r",    "bt_l_u_d", "bt_l_l_r", "key_door",   "bt_r_stick",  "bt_l_stick",  "bt_r_stick_press",  "bt_l_stick_press", "",            "",            "",       "",     ""
};
// clang-format on

// clang-format off
static const std::map<std::string, std::string> key_controller_map{
   {"key_cursor_u", "dpad_u"},
   {"key_cursor_d", "dpad_d"},
   {"key_cursor_l", "dpad_l"},
   {"key_cursor_r", "dpad_r"},
   {"key_space",    "bt_a"},
   {"key_return",   "bt_b"},
   {"key_escape",   "bt_b"},
};
// clang-format on

/// \brief size variant an icon is requested in.
///
/// the atlas ships every glyph twice. the large glyphs are 18x18 inside their cell and are meant for
/// the world space hints, the small ones are 14x15 and are meant for the hud, where they have to fit
/// into a 38px item frame.
enum class IconSize
{
   Large,
   Small,
};

/// \brief controller family whose button artwork is used.
///
/// there is no Keyboard entry on purpose: which of the keyboard and controller icon a caller wants is
/// already decided by the icon id it passes in, see retrieveMappedKey.
enum class IconBrand
{
   Xbox,
   PlayStation,
   Switch,
   SteamDeck,
};

/// \brief brand used when no controller is connected or its hardware is not recognized.
static constexpr auto default_icon_brand = IconBrand::Xbox;

/// \brief one icon id and the atlas cell that holds its artwork.
struct IconCell
{
   std::string_view _icon_id;  //!< logical icon id, the same namespace key_map and the tmx properties use
   int32_t _column{0};         //!< column in the ui icon atlas grid
   int32_t _row{0};            //!< row in the ui icon atlas grid
};

// the atlas is data/game/ui_icons.png, a 20x22 grid of PIXELS_PER_TILE sized cells. key_map above
// covers rows 0 to 8, which is the keyboard, the xbox pad and the generic pad artwork in their large
// size, and is indexed by its own 16 column stride. everything the brand tables below point at lives
// outside that stride, so it can only be reached through them:
//
//   rows  0 -  4, cols  0 - 12   keyboard, large              (key_map rows 0 - 4)
//   rows  0 -  2, cols 13 - 18   steam deck, large            (steam_deck_large_map)
//   rows  3 -  4, cols 13 - 18   playstation, large           (playstation_large_map)
//   row   5,      cols  0 -  9   xbox, large                  (key_map row 5)
//   rows  6 -  8, cols  0 - 15   generic pad, large           (key_map rows 6 - 8)
//   rows  9 - 11, cols  0 - 11   text arrows and help bubbles (not addressed by id, see ControllerHelp)
//   row  12,      cols  0 - 13   switch, large                (switch_large_map)
//   rows 13 - 17, cols  0 - 12   keyboard, small              (key_map rows 0 - 4 plus keyboard_small_row_offset)
//   row  18,      cols  0 -  9   switch, small                (switch_small_map)
//   row  19,      cols  0 -  9   xbox, small                  (xbox_small_map)
//   row  20,      cols  0 -  9   playstation, small           (playstation_small_map)
//   row  21,      cols  0 - 14   steam deck, small            (steam_deck_small_map)
//
// note that the small pad rows are not row shifted copies of their large counterparts. the xbox row
// swaps the shoulder and the trigger pair, and the switch row drops the dpad and moves plus and minus,
// so every brand block is spelled out rather than derived.

/// \brief rows between a large keyboard cell and its small counterpart.
static constexpr auto keyboard_small_row_offset = 13;

/// \brief keyboard icon ids that the atlas draws large but not small.
static constexpr std::array<std::string_view, 1> keyboard_ids_without_small_cell{
   "key_0",
};

// clang-format off
static constexpr std::array<IconCell, 10> xbox_small_map{{
   {"bt_a", 0, 19}, {"bt_b", 1, 19}, {"bt_x", 2, 19}, {"bt_y", 3, 19}, {"bt_list", 4, 19},
   {"bt_menu", 5, 19}, {"bt_rb", 6, 19}, {"bt_lb", 7, 19}, {"bt_rt", 8, 19}, {"bt_lt", 9, 19},
}};

// plus is the start button and minus the select button, so they take the bt_list and bt_menu ids
static constexpr std::array<IconCell, 14> switch_large_map{{
   {"bt_a", 0, 12}, {"bt_b", 1, 12}, {"bt_x", 2, 12}, {"bt_y", 3, 12},
   {"dpad_u", 4, 12}, {"dpad_d", 5, 12}, {"dpad_l", 6, 12}, {"dpad_r", 7, 12},
   {"bt_list", 8, 12}, {"bt_menu", 9, 12},
   {"bt_rt", 10, 12}, {"bt_lt", 11, 12}, {"bt_rb", 12, 12}, {"bt_lb", 13, 12},
}};

// zr and zl are the triggers, r and l the shoulders. this row has no dpad, so a dpad icon falls
// through to switch_large_map
static constexpr std::array<IconCell, 10> switch_small_map{{
   {"bt_a", 0, 18}, {"bt_b", 1, 18}, {"bt_x", 2, 18}, {"bt_y", 3, 18},
   {"bt_list", 4, 18}, {"bt_menu", 5, 18},
   {"bt_rt", 6, 18}, {"bt_lt", 7, 18}, {"bt_rb", 8, 18}, {"bt_lb", 9, 18},
}};

// options carries the three line glyph and therefore the bt_list id, create the bt_menu one.
// r1 and l1 are the shoulders, r2 and l2 the triggers
static constexpr std::array<IconCell, 10> playstation_large_map{{
   {"bt_a", 13, 3}, {"bt_b", 14, 3}, {"bt_x", 15, 3}, {"bt_y", 16, 3}, {"bt_menu", 17, 3}, {"bt_list", 18, 3},
   {"bt_rb", 13, 4}, {"bt_lb", 14, 4}, {"bt_rt", 15, 4}, {"bt_lt", 16, 4},
}};

static constexpr std::array<IconCell, 10> playstation_small_map{{
   {"bt_a", 0, 20}, {"bt_b", 1, 20}, {"bt_x", 2, 20}, {"bt_y", 3, 20}, {"bt_menu", 4, 20}, {"bt_list", 5, 20},
   {"bt_rb", 6, 20}, {"bt_lb", 7, 20}, {"bt_rt", 8, 20}, {"bt_lt", 9, 20},
}};

// r4, l4, r5 and l5 are the four buttons on the back of the deck
static constexpr std::array<IconCell, 14> steam_deck_large_map{{
   {"bt_a", 13, 0}, {"bt_b", 14, 0}, {"bt_x", 15, 0}, {"bt_y", 16, 0}, {"bt_list", 17, 0}, {"bt_menu", 18, 0},
   {"bt_rb", 13, 1}, {"bt_lb", 14, 1}, {"bt_rt", 15, 1}, {"bt_lt", 16, 1},
   {"bt_r4", 13, 2}, {"bt_l4", 14, 2}, {"bt_r5", 15, 2}, {"bt_l5", 16, 2},
}};

// bt_quick_access has no large counterpart, so it only ever resolves for a small request
static constexpr std::array<IconCell, 15> steam_deck_small_map{{
   {"bt_a", 0, 21}, {"bt_b", 1, 21}, {"bt_x", 2, 21}, {"bt_y", 3, 21},
   {"bt_quick_access", 4, 21}, {"bt_list", 5, 21}, {"bt_menu", 6, 21},
   {"bt_rb", 7, 21}, {"bt_lb", 8, 21}, {"bt_rt", 9, 21}, {"bt_lt", 10, 21},
   {"bt_r4", 11, 21}, {"bt_l4", 12, 21}, {"bt_r5", 13, 21}, {"bt_l5", 14, 21},
}};
// clang-format on

/// \brief resolves an icon id to its column and row in the ui icon atlas grid.
///
/// the requested brand and size are tried first, then the same brand in its large size, then the
/// keyboard block, then key_map, which holds the keyboard, xbox and generic pad artwork in their large
/// size. a large request never falls back to a small cell, so a glyph the atlas only draws small stays
/// unresolved rather than being drawn at the wrong size.
///
/// \param icon_id logical icon name, for example "key_space" or "bt_a".
/// \param brand controller family whose artwork is preferred.
/// \param size size variant to look for.
/// \return {column, row} tile coordinates, or nothing when the atlas has no cell for the icon.
std::optional<std::pair<int32_t, int32_t>>
getArrayPosition(const std::string& icon_id, IconBrand brand = default_icon_brand, IconSize size = IconSize::Large);

/// \brief returns keyboard and controller key ids for one logical input.
/// \param key keyboard key id to map.
/// \return pair {keyboard_key, controller_key}; falls back to {key, key} when no mapping exists.
std::pair<std::string, std::string> retrieveMappedKey(const std::string& key);

/// \brief resolves a keyboard key to the icon id that depicts its keycap.
/// \param key keyboard key to depict.
/// \return icon id such as "key_ctrl", or an empty string when the atlas has no keycap for the key.
std::string iconNameForKeyboardKey(sf::Keyboard::Key key);

/// \brief resolves a controller button to the icon id that depicts it.
/// \param sdl_button SDL gamepad button index to depict.
/// \return icon id such as "bt_x", or an empty string when the atlas has no icon for the button.
std::string iconNameForControllerButton(int32_t sdl_button);

/// \brief returns the icon brand that matches the hardware of the connected controller.
/// \return brand for the connected controller, or default_icon_brand when none is connected or the
///         hardware is not recognized. the switch build always reports IconBrand::Switch.
IconBrand brandForConnectedController();

}  // namespace ControllerKeyMap

#endif  // CONTROLLERKEYMAP_H
