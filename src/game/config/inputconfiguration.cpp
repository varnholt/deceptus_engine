#include "inputconfiguration.h"

#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>

#include "framework/tools/log.h"
#include "json/json.hpp"

// SDL
#ifdef TARGET_OS_MAC
#ifdef MAC_OS_X_VERSION_MIN_REQUIRED
#undef MAC_OS_X_VERSION_MIN_REQUIRED
#endif
#define MAC_OS_X_VERSION_MIN_REQUIRED MAC_OS_X_VERSION_10_9
#endif
#include <SDL3/SDL.h>

using json = nlohmann::json;

namespace
{

const std::map<std::string, KeyPressed>& getActionNameToFlagMap()
{
   static const std::map<std::string, KeyPressed> action_name_to_flag = {
      {"up", KeyPressedUp},
      {"down", KeyPressedDown},
      {"left", KeyPressedLeft},
      {"right", KeyPressedRight},
      {"jump", KeyPressedJump},
      {"slot_1", KeyPressedSlot1},
      {"slot_2", KeyPressedSlot2},
      {"look", KeyPressedLook},
      {"action", KeyPressedAction},
      {"inventory", KeyPressedInventory},
   };
   return action_name_to_flag;
}

const std::map<KeyPressed, std::string>& getActionFlagToNameMap()
{
   static const auto action_flag_to_name = []()
   {
      std::map<KeyPressed, std::string> result;
      for (const auto& [name, flag] : getActionNameToFlagMap())
      {
         result[flag] = name;
      }
      return result;
   }();
   return action_flag_to_name;
}

const std::map<std::string, sf::Keyboard::Key>& getKeyboardNameToKeyMap()
{
   static const std::map<std::string, sf::Keyboard::Key> keyboard_name_to_key = {
      {"A", sf::Keyboard::Key::A},
      {"B", sf::Keyboard::Key::B},
      {"C", sf::Keyboard::Key::C},
      {"D", sf::Keyboard::Key::D},
      {"E", sf::Keyboard::Key::E},
      {"F", sf::Keyboard::Key::F},
      {"G", sf::Keyboard::Key::G},
      {"H", sf::Keyboard::Key::H},
      {"I", sf::Keyboard::Key::I},
      {"J", sf::Keyboard::Key::J},
      {"K", sf::Keyboard::Key::K},
      {"L", sf::Keyboard::Key::L},
      {"M", sf::Keyboard::Key::M},
      {"N", sf::Keyboard::Key::N},
      {"O", sf::Keyboard::Key::O},
      {"P", sf::Keyboard::Key::P},
      {"Q", sf::Keyboard::Key::Q},
      {"R", sf::Keyboard::Key::R},
      {"S", sf::Keyboard::Key::S},
      {"T", sf::Keyboard::Key::T},
      {"U", sf::Keyboard::Key::U},
      {"V", sf::Keyboard::Key::V},
      {"W", sf::Keyboard::Key::W},
      {"X", sf::Keyboard::Key::X},
      {"Y", sf::Keyboard::Key::Y},
      {"Z", sf::Keyboard::Key::Z},
      {"Num0", sf::Keyboard::Key::Num0},
      {"Num1", sf::Keyboard::Key::Num1},
      {"Num2", sf::Keyboard::Key::Num2},
      {"Num3", sf::Keyboard::Key::Num3},
      {"Num4", sf::Keyboard::Key::Num4},
      {"Num5", sf::Keyboard::Key::Num5},
      {"Num6", sf::Keyboard::Key::Num6},
      {"Num7", sf::Keyboard::Key::Num7},
      {"Num8", sf::Keyboard::Key::Num8},
      {"Num9", sf::Keyboard::Key::Num9},
      {"Escape", sf::Keyboard::Key::Escape},
      {"LControl", sf::Keyboard::Key::LControl},
      {"LShift", sf::Keyboard::Key::LShift},
      {"LAlt", sf::Keyboard::Key::LAlt},
      {"LSystem", sf::Keyboard::Key::LSystem},
      {"RControl", sf::Keyboard::Key::RControl},
      {"RShift", sf::Keyboard::Key::RShift},
      {"RAlt", sf::Keyboard::Key::RAlt},
      {"RSystem", sf::Keyboard::Key::RSystem},
      {"Menu", sf::Keyboard::Key::Menu},
      {"LBracket", sf::Keyboard::Key::LBracket},
      {"RBracket", sf::Keyboard::Key::RBracket},
      {"Semicolon", sf::Keyboard::Key::Semicolon},
      {"Comma", sf::Keyboard::Key::Comma},
      {"Period", sf::Keyboard::Key::Period},
      {"Apostrophe", sf::Keyboard::Key::Apostrophe},
      {"Slash", sf::Keyboard::Key::Slash},
      {"Backslash", sf::Keyboard::Key::Backslash},
      {"Grave", sf::Keyboard::Key::Grave},
      {"Equal", sf::Keyboard::Key::Equal},
      {"Hyphen", sf::Keyboard::Key::Hyphen},
      {"Space", sf::Keyboard::Key::Space},
      {"Enter", sf::Keyboard::Key::Enter},
      {"Backspace", sf::Keyboard::Key::Backspace},
      {"Tab", sf::Keyboard::Key::Tab},
      {"PageUp", sf::Keyboard::Key::PageUp},
      {"PageDown", sf::Keyboard::Key::PageDown},
      {"End", sf::Keyboard::Key::End},
      {"Home", sf::Keyboard::Key::Home},
      {"Insert", sf::Keyboard::Key::Insert},
      {"Delete", sf::Keyboard::Key::Delete},
      {"Add", sf::Keyboard::Key::Add},
      {"Subtract", sf::Keyboard::Key::Subtract},
      {"Multiply", sf::Keyboard::Key::Multiply},
      {"Divide", sf::Keyboard::Key::Divide},
      {"Left", sf::Keyboard::Key::Left},
      {"Right", sf::Keyboard::Key::Right},
      {"Up", sf::Keyboard::Key::Up},
      {"Down", sf::Keyboard::Key::Down},
      {"Numpad0", sf::Keyboard::Key::Numpad0},
      {"Numpad1", sf::Keyboard::Key::Numpad1},
      {"Numpad2", sf::Keyboard::Key::Numpad2},
      {"Numpad3", sf::Keyboard::Key::Numpad3},
      {"Numpad4", sf::Keyboard::Key::Numpad4},
      {"Numpad5", sf::Keyboard::Key::Numpad5},
      {"Numpad6", sf::Keyboard::Key::Numpad6},
      {"Numpad7", sf::Keyboard::Key::Numpad7},
      {"Numpad8", sf::Keyboard::Key::Numpad8},
      {"Numpad9", sf::Keyboard::Key::Numpad9},
      {"F1", sf::Keyboard::Key::F1},
      {"F2", sf::Keyboard::Key::F2},
      {"F3", sf::Keyboard::Key::F3},
      {"F4", sf::Keyboard::Key::F4},
      {"F5", sf::Keyboard::Key::F5},
      {"F6", sf::Keyboard::Key::F6},
      {"F7", sf::Keyboard::Key::F7},
      {"F8", sf::Keyboard::Key::F8},
      {"F9", sf::Keyboard::Key::F9},
      {"F10", sf::Keyboard::Key::F10},
      {"F11", sf::Keyboard::Key::F11},
      {"F12", sf::Keyboard::Key::F12},
      {"Pause", sf::Keyboard::Key::Pause},
   };
   return keyboard_name_to_key;
}

const std::map<sf::Keyboard::Key, std::string>& getKeyToKeyboardNameMap()
{
   static const auto key_to_name = []()
   {
      std::map<sf::Keyboard::Key, std::string> result;
      for (const auto& [name, key] : getKeyboardNameToKeyMap())
      {
         result[key] = name;
      }
      return result;
   }();
   return key_to_name;
}

const std::map<std::string, int32_t>& getControllerButtonNameToSdlMap()
{
   static const std::map<std::string, int32_t> controller_button_name_to_sdl = {
      {"South", SDL_GAMEPAD_BUTTON_SOUTH},
      {"East", SDL_GAMEPAD_BUTTON_EAST},
      {"West", SDL_GAMEPAD_BUTTON_WEST},
      {"North", SDL_GAMEPAD_BUTTON_NORTH},
      {"Back", SDL_GAMEPAD_BUTTON_BACK},
      {"Guide", SDL_GAMEPAD_BUTTON_GUIDE},
      {"Start", SDL_GAMEPAD_BUTTON_START},
      {"LeftStick", SDL_GAMEPAD_BUTTON_LEFT_STICK},
      {"RightStick", SDL_GAMEPAD_BUTTON_RIGHT_STICK},
      {"LeftShoulder", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER},
      {"RightShoulder", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER},
      {"DpadUp", SDL_GAMEPAD_BUTTON_DPAD_UP},
      {"DpadDown", SDL_GAMEPAD_BUTTON_DPAD_DOWN},
      {"DpadLeft", SDL_GAMEPAD_BUTTON_DPAD_LEFT},
      {"DpadRight", SDL_GAMEPAD_BUTTON_DPAD_RIGHT},
   };
   return controller_button_name_to_sdl;
}

const std::map<int32_t, std::string>& getSdlToControllerButtonNameMap()
{
   static const auto sdl_to_name = []()
   {
      std::map<int32_t, std::string> result;
      for (const auto& [name, button] : getControllerButtonNameToSdlMap())
      {
         result[button] = name;
      }
      return result;
   }();
   return sdl_to_name;
}

bool input_configuration_initialized = false;
InputConfiguration input_configuration_defaults;

}  // namespace

void InputConfiguration::deserializeControllerSection(const std::string& data)
{
   const auto& action_name_to_flag = getActionNameToFlagMap();
   const auto& controller_button_name_to_sdl = getControllerButtonNameToSdlMap();

   try
   {
      json config = json::parse(data);
      const auto& controls = config["Controls"];
      if (!controls.count("controller"))
      {
         return;
      }

      const auto& controller_json = controls["controller"];
      for (const auto& [action_name, button_name_value] : controller_json.items())
      {
         const auto button_name = button_name_value.get<std::string>();
         const auto action_flag_it = action_name_to_flag.find(action_name);
         const auto button_it = controller_button_name_to_sdl.find(button_name);
         if (action_flag_it != action_name_to_flag.end() && button_it != controller_button_name_to_sdl.end())
         {
            _action_to_controller_button[action_flag_it->second] = button_it->second;
         }
         else
         {
            Log::Warning() << "controller config: unknown action or button name: " << action_name << " -> " << button_name;
         }
      }
   }
   catch (const std::exception& exception)
   {
      Log::Error() << "failed to parse controller config: " << exception.what();
   }
}

std::string InputConfiguration::serializeControllerSection() const
{
   const auto& action_flag_to_name = getActionFlagToNameMap();
   const auto& sdl_to_controller_button_name = getSdlToControllerButtonNameMap();

   json controller_json;
   for (const auto& [action_flag, sdl_button] : _action_to_controller_button)
   {
      const auto action_name_it = action_flag_to_name.find(action_flag);
      const auto button_name_it = sdl_to_controller_button_name.find(sdl_button);
      if (action_name_it != action_flag_to_name.end() && button_name_it != sdl_to_controller_button_name.end())
      {
         controller_json[action_name_it->second] = button_name_it->second;
      }
   }

   json config = {
      {"Controls",
       {
          {"controller", controller_json},
       }}
   };

   std::stringstream sstream;
   sstream << std::setw(4) << config << "\n\n";
   return sstream.str();
}

void InputConfiguration::setDefaults()
{
   // default bindings:
   //
   //   action   keyboard     controller button
   //   -------  -----------  -----------------------------------------
   //   up       Up           DpadUp
   //   down     Down         DpadDown
   //   left     Left         (analog stick / d-pad hat only, no button)
   //   right    Right        (analog stick / d-pad hat only, no button)
   //   jump     Space        South (A)
   //   slot_1   LControl     West  (X)
   //   slot_2   LAlt         North (Y)
   //   look     LShift       (right analog stick only, no button)
   //   action   Enter        East  (B)

   _action_to_key = {
      {KeyPressedUp, sf::Keyboard::Key::Up},
      {KeyPressedDown, sf::Keyboard::Key::Down},
      {KeyPressedLeft, sf::Keyboard::Key::Left},
      {KeyPressedRight, sf::Keyboard::Key::Right},
      {KeyPressedJump, sf::Keyboard::Key::Space},
      {KeyPressedSlot1, sf::Keyboard::Key::LControl},
      {KeyPressedSlot2, sf::Keyboard::Key::LAlt},
      {KeyPressedLook, sf::Keyboard::Key::LShift},
      {KeyPressedAction, sf::Keyboard::Key::Enter},
      {KeyPressedInventory, sf::Keyboard::Key::Tab},
   };

   _key_to_action.clear();
   for (const auto& [action_flag, keyboard_key] : _action_to_key)
   {
      _key_to_action[keyboard_key] = action_flag;
   }

   _action_to_controller_button = {
      {KeyPressedJump, SDL_GAMEPAD_BUTTON_SOUTH},
      {KeyPressedAction, SDL_GAMEPAD_BUTTON_EAST},
      {KeyPressedSlot1, SDL_GAMEPAD_BUTTON_WEST},
      {KeyPressedSlot2, SDL_GAMEPAD_BUTTON_NORTH},
      {KeyPressedUp, SDL_GAMEPAD_BUTTON_DPAD_UP},
      {KeyPressedDown, SDL_GAMEPAD_BUTTON_DPAD_DOWN},
      {KeyPressedLeft, SDL_GAMEPAD_BUTTON_DPAD_LEFT},
      {KeyPressedRight, SDL_GAMEPAD_BUTTON_DPAD_RIGHT},
      {KeyPressedInventory, SDL_GAMEPAD_BUTTON_BACK},
   };
}

std::string InputConfiguration::serialize() const
{
   const auto& action_flag_to_name = getActionFlagToNameMap();
   const auto& key_to_keyboard_name = getKeyToKeyboardNameMap();
   const auto& sdl_to_controller_button_name = getSdlToControllerButtonNameMap();

   json keyboard_json;
   for (const auto& [action_flag, keyboard_key] : _action_to_key)
   {
      const auto action_name_it = action_flag_to_name.find(action_flag);
      const auto key_name_it = key_to_keyboard_name.find(keyboard_key);
      if (action_name_it != action_flag_to_name.end() && key_name_it != key_to_keyboard_name.end())
      {
         keyboard_json[action_name_it->second] = key_name_it->second;
      }
   }

   json controller_json;
   for (const auto& [action_flag, sdl_button] : _action_to_controller_button)
   {
      const auto action_name_it = action_flag_to_name.find(action_flag);
      const auto button_name_it = sdl_to_controller_button_name.find(sdl_button);
      if (action_name_it != action_flag_to_name.end() && button_name_it != sdl_to_controller_button_name.end())
      {
         controller_json[action_name_it->second] = button_name_it->second;
      }
   }

   json config = {
      {"Controls",
       {
          {"keyboard", keyboard_json},
          {"controller", controller_json},
       }}
   };

   std::stringstream sstream;
   sstream << std::setw(4) << config << "\n\n";
   return sstream.str();
}

void InputConfiguration::deserialize(const std::string& data)
{
   const auto& action_name_to_flag = getActionNameToFlagMap();
   const auto& keyboard_name_to_key = getKeyboardNameToKeyMap();
   const auto& controller_button_name_to_sdl = getControllerButtonNameToSdlMap();

   try
   {
      json config = json::parse(data);
      const auto& controls = config["Controls"];

      const auto& keyboard_json = controls["keyboard"];
      for (const auto& [action_name, key_name_value] : keyboard_json.items())
      {
         const auto key_name = key_name_value.get<std::string>();
         const auto action_flag_it = action_name_to_flag.find(action_name);
         const auto key_it = keyboard_name_to_key.find(key_name);
         if (action_flag_it != action_name_to_flag.end() && key_it != keyboard_name_to_key.end())
         {
            const auto action_flag = action_flag_it->second;
            const auto keyboard_key = key_it->second;

            // remove any existing reverse mapping for the old key bound to this action
            const auto previous_key_it = _action_to_key.find(action_flag);
            if (previous_key_it != _action_to_key.end())
            {
               _key_to_action.erase(previous_key_it->second);
            }

            _action_to_key[action_flag] = keyboard_key;
            _key_to_action[keyboard_key] = action_flag;
         }
         else
         {
            Log::Warning() << "controls.json: unknown action or key name: " << action_name << " -> " << key_name;
         }
      }

      const auto& controller_json = controls["controller"];
      for (const auto& [action_name, button_name_value] : controller_json.items())
      {
         const auto button_name = button_name_value.get<std::string>();
         const auto action_flag_it = action_name_to_flag.find(action_name);
         const auto button_it = controller_button_name_to_sdl.find(button_name);
         if (action_flag_it != action_name_to_flag.end() && button_it != controller_button_name_to_sdl.end())
         {
            _action_to_controller_button[action_flag_it->second] = button_it->second;
         }
         else
         {
            Log::Warning() << "controls.json: unknown action or button name: " << action_name << " -> " << button_name;
         }
      }
   }
   catch (const std::exception& exception)
   {
      Log::Error() << "failed to parse controls.json: " << exception.what();
      setDefaults();
   }
}

void InputConfiguration::deserializeFromFile(const std::string& filename)
{
   std::ifstream ifs(filename, std::ifstream::in);

   if (!ifs.good())
   {
      Log::Info() << "controls file not found, writing defaults to " << filename;
      setDefaults();
      serializeToFile(filename);
      return;
   }

   char character = static_cast<char>(ifs.get());
   std::string data;

   while (ifs.good())
   {
      data.push_back(character);
      character = static_cast<char>(ifs.get());
   }

   ifs.close();

   setDefaults();
   deserialize(data);
}

void InputConfiguration::deserializeFromFile()
{
   deserializeFromFile(_current_filename);
}

void InputConfiguration::serializeToFile(const std::string& filename)
{
   std::string data = serialize();
   std::ofstream output_file(filename);
   output_file << data;
}

void InputConfiguration::serializeToFile()
{
   serializeToFile(_current_filename);
}

void InputConfiguration::mergeControllerBindingsFromFile(const std::string& filename)
{
   std::ifstream ifs(filename, std::ifstream::in);
   if (!ifs.good())
   {
      return;
   }

   char character = static_cast<char>(ifs.get());
   std::string data;

   while (ifs.good())
   {
      data.push_back(character);
      character = static_cast<char>(ifs.get());
   }

   ifs.close();
   deserializeControllerSection(data);
}

void InputConfiguration::saveControllerBindingsToFile(const std::string& filename)
{
   std::string data = serializeControllerSection();
   std::ofstream output_file(filename);
   output_file << data;
}

void InputConfiguration::setCurrentFilename(const std::string& filename)
{
   _current_filename = filename;
}

const std::string& InputConfiguration::getCurrentFilename() const
{
   return _current_filename;
}

std::string InputConfiguration::keyboardFilename()
{
   return "data/config/controls.json";
}

std::string InputConfiguration::controllerFilename(const std::string& guid)
{
   return "data/config/controls_controller_" + guid + ".json";
}

InputConfiguration& InputConfiguration::getDefaults()
{
   return input_configuration_defaults;
}

std::string InputConfiguration::keyName(sf::Keyboard::Key key)
{
   const auto& key_to_name = getKeyToKeyboardNameMap();
   const auto found_entry = key_to_name.find(key);
   if (found_entry != key_to_name.end())
   {
      return found_entry->second;
   }
   return "--";
}

std::string InputConfiguration::buttonName(int32_t sdl_button)
{
   const auto& sdl_to_name = getSdlToControllerButtonNameMap();
   const auto found_entry = sdl_to_name.find(sdl_button);
   if (found_entry != sdl_to_name.end())
   {
      return found_entry->second;
   }
   return "--";
}

std::string InputConfiguration::actionDisplayName(KeyPressed action)
{
   static const std::map<KeyPressed, std::string> action_to_display_name = {
      {KeyPressedUp, "Up"},
      {KeyPressedDown, "Down"},
      {KeyPressedLeft, "Left"},
      {KeyPressedRight, "Right"},
      {KeyPressedJump, "Jump"},
      {KeyPressedSlot1, "Slot 1"},
      {KeyPressedSlot2, "Slot 2"},
      {KeyPressedLook, "Look"},
      {KeyPressedAction, "Action"},
      {KeyPressedInventory, "Inventory"},
   };
   const auto found_entry = action_to_display_name.find(action);
   if (found_entry != action_to_display_name.end())
   {
      return found_entry->second;
   }
   return "Unknown";
}

const std::vector<KeyPressed>& InputConfiguration::actionList()
{
   static const std::vector<KeyPressed> action_list = {
      KeyPressedUp,
      KeyPressedDown,
      KeyPressedLeft,
      KeyPressedRight,
      KeyPressedJump,
      KeyPressedSlot1,
      KeyPressedSlot2,
      KeyPressedLook,
      KeyPressedAction,
      KeyPressedInventory,
   };
   return action_list;
}

InputConfiguration& InputConfiguration::getInstance()
{
   static InputConfiguration __instance;

   if (!input_configuration_initialized)
   {
      __instance.setDefaults();
      input_configuration_defaults.setDefaults();
      __instance._current_filename = keyboardFilename();
      __instance.deserializeFromFile(keyboardFilename());
      input_configuration_initialized = true;
   }

   return __instance;
}
