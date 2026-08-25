#include "controllerkeymap.h"

#include <algorithm>

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
      {SDL_GAMEPAD_BUTTON_BACK, "bt_list"},
      {SDL_GAMEPAD_BUTTON_START, "bt_menu"},
      {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, "bt_lb"},
      {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, "bt_rb"},
      {SDL_GAMEPAD_BUTTON_DPAD_UP, "dpad_u"},
      {SDL_GAMEPAD_BUTTON_DPAD_DOWN, "dpad_d"},
      {SDL_GAMEPAD_BUTTON_DPAD_LEFT, "dpad_l"},
      {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, "dpad_r"},
   };
   return controller_button_to_icon;
}

}  // namespace

std::pair<int32_t, int32_t> ControllerKeyMap::getArrayPosition(const std::string& key)
{
   const auto it = std::find(key_map.begin(), key_map.end(), key);
   const auto index = static_cast<int32_t>(std::distance(key_map.begin(), it));

   const auto row = index / 16;
   const auto col = index % 16;

   return {col, row};
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
