#pragma once

#include "game/constants.h"

#include <SFML/Graphics.hpp>
#include <map>
#include <string>

/// \brief stores player input key and controller button bindings and handles json persistence.
struct InputConfiguration
{
   // sf::Keyboard::Key is a scoped enum (enum class) in SFML 3 — no implicit integer conversion, so
   // std::unordered_map would require a custom hasher. std::map works because scoped enums support
   // operator< via their underlying integral type. KeyPressed is a plain enum and would work with
   // either; std::map is used throughout for consistency.
   std::map<sf::Keyboard::Key, KeyPressed> _key_to_action;      //!< reverse keyboard lookup (key → action): used in keyboardKeyPressed / keyboardKeyReleased
   std::map<KeyPressed, sf::Keyboard::Key> _action_to_key;      //!< forward keyboard lookup (action → key): used in forceSync to call sf::Keyboard::isKeyPressed per bound action
   std::map<KeyPressed, int32_t> _action_to_controller_button;  //!< controller lookup (action → SDL button index): used in isControllerActionPressed

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

private:
   void deserialize(const std::string& data);
   std::string serialize() const;
   void setDefaults();
};
