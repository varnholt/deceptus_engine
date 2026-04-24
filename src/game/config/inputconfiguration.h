#pragma once

#include "game/constants.h"

#include <SFML/Graphics.hpp>
#include <map>
#include <string>
#include <vector>

/// \brief stores player input key and controller button bindings and handles json persistence.
struct InputConfiguration
{
   // sf::Keyboard::Key is a scoped enum (enum class) in SFML 3 — no implicit integer conversion, so
   // std::unordered_map would require a custom hasher. std::map works because scoped enums support
   // operator< via their underlying integral type. KeyPressed is a plain enum and would work with
   // either; std::map is used throughout for consistency.
   std::map<sf::Keyboard::Key, KeyPressed>
      _key_to_action;  //!< reverse keyboard lookup (key → action): used in keyboardKeyPressed / keyboardKeyReleased
   std::map<KeyPressed, sf::Keyboard::Key>
      _action_to_key;  //!< forward keyboard lookup (action → key): used in forceSync to call sf::Keyboard::isKeyPressed per bound action
   std::map<KeyPressed, int32_t>
      _action_to_controller_button;  //!< controller lookup (action → SDL button index): used in isControllerActionPressed

   /// \brief loads bindings from a json file, falling back to defaults for missing entries.
   /// \param filename source configuration file path.
   void deserializeFromFile(const std::string& filename = "data/config/controls.json");

   /// \brief writes current bindings to a json file.
   /// \param filename destination configuration file path.
   void serializeToFile(const std::string& filename = "data/config/controls.json");

   /// \brief returns the built-in default bindings.
   /// \return shared default configuration object.
   static InputConfiguration& getDefaults();

   /// \brief returns the active input configuration, loading from disk on first access.
   /// \return singleton runtime configuration object.
   static InputConfiguration& getInstance();

   /// \brief returns the display name of a keyboard key (e.g. "Space", "Left").
   /// \param key keyboard key to look up.
   /// \return name string, or "--" when the key is not in the map.
   static std::string keyName(sf::Keyboard::Key key);

   /// \brief returns the display name of an SDL gamepad button (e.g. "South", "West").
   /// \param sdl_button SDL button index to look up.
   /// \return name string, or "--" when the button is not in the map.
   static std::string buttonName(int32_t sdl_button);

   /// \brief returns the human-readable display name for a game action.
   /// \param action action flag to look up.
   /// \return display name string (e.g. "Jump", "Slot 1").
   static std::string actionDisplayName(KeyPressed action);

   /// \brief returns the ordered list of all game actions used for input assignment.
   /// \return reference to a static vector of all KeyPressed action flags.
   static const std::vector<KeyPressed>& actionList();

private:
   void deserialize(const std::string& data);
   std::string serialize() const;
   void setDefaults();
};
