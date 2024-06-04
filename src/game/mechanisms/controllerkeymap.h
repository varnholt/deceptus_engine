#ifndef CONTROLLERKEYMAP_H
#define CONTROLLERKEYMAP_H

#include <array>
#include <map>
#include <string>

namespace ControllerKeyMap
{

// clang-format off
static const std::array<std::string, 16 * 9> key_map{
   "key_esc",   "key_1",   "key_2",   "key_3",     "key_4",    "key_5",   "key_6",       "key_7", "key_8", "key_9",        "key_minus",     "key_equals",     "key_backspace", "",     "",       "",
   "key_tab",   "key_q",   "key_w",   "key_e",     "key_r",    "key_t",   "key_y",       "key_u", "key_i", "key_o",        "key_p",         "key_bracket_l",  "key_bracket_r", "",     "",       "",
   "key_caps",  "key_a",   "key_s",   "key_d",     "key_f",    "key_g",   "key_h",       "key_j", "key_k", "key_l",        "key_semicolon", "key_apostrophe", "key_return",    "",     "",       "",
   "key_shift", "key_0",   "key_z",   "key_x",     "key_c",    "key_v",   "key_b",       "key_n", "key_m", "key_comma",    "key_period",    "key_question",   "key_backslash", "",     "",       "",
   "key_ctrl",  "key_win", "key_alt", "key_empty", "key_list",  "",        "",           "",      "",      "key_cursor_l", "key_cursor_u",  "key_cursor_d",   "key_cursor_r",  "",     "",       "",
   "bt_a",      "bt_b",    "bt_x",    "bt_y",      "bt_list",  "bt_menu",  "bt_rt",      "bt_lt", "bt_lb", "bt_rb",         "",              "",               "",             "",     "",       "",
   "dpad_u",    "dpad_d",  "dpad_l",  "dpad_r",    "bt_u",     "bt_d",     "bt_l",       "bt_r",  "bt_1",  "bt_2",          "bt_3",          "bt_4",           "bt_5",        "bt_6",  "bt_7",  "bt_8",
   "bt_r_u",    "bt_r_d",  "bt_r_l",  "bt_r_r",    "bt_r_u_d", "bt_r_l_r", "dpad_empty", "bt_0",  "bt_9",  "bt_10",         "bt_11",         "bt_12",          "bt_13",       "bt_14", "bt_15", "bt_16",
   "bt_l_u",    "bt_l_d",  "bt_l_l",  "bt_l_r",    "bt_l_u_d", "bt_l_l_r", "key_door",   "",      "",     "",              "",               "",              "",            "",       "",     ""
};
// clang-format on

// clang-format off
std::map<std::string, std::string> key_controller_map{
   {"key_cursor_u", "dpad_u"},
   {"key_cursor_d", "dpad_d"},
   {"key_cursor_l", "dpad_l"},
   {"key_cursor_r", "dpad_r"},
   {"key_return",   "bt_a"},
   {"key_escape",   "bt_b"},
};
// clang-format on

std::pair<int32_t, int32_t> getArrayPosition(const std::string& key);

}  // namespace ControllerKeyMap

#endif  // CONTROLLERKEYMAP_H
