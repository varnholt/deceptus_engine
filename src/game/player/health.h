#pragma once

#include <cstdint>

#include <SFML/Graphics.hpp>

#include "json/json.hpp"

/// \brief stores player life, health, and stamina state with stamina-drain flags.
struct Health
{
   /// \brief creates health state with default life, health, and full stamina.
   Health() = default;

   /// \brief bit flags for systems that currently consume stamina.
   enum class StaminaDrain
   {
      None = 0x0,
      Dash = 0x1
   };

   /// \brief resets health and stamina to runtime defaults.
   void reset();

   /// \brief adds health points and clamps the result to _health_max.
   /// \param health amount of health to add.
   void addHealth(int32_t health);

   /// \brief updates stamina by draining or recharging based on active drain flags.
   /// \param dt elapsed frame time used for stamina integration.
   void update(const sf::Time& dt);

   /// \brief enables a stamina-drain source flag.
   /// \param drain drain flag to add to the active bitmask.
   void addStaminaDrain(StaminaDrain);

   /// \brief disables a stamina-drain source flag.
   /// \param drain drain flag to remove from the active bitmask.
   void removeStaminaDrain(StaminaDrain);

   /// \brief checks whether stamina is effectively full.
   /// \return true when stamina is above 0.999f.
   bool hasFullStamina() const;

   int32_t _life_count = 1;

   int32_t _health = 4;
   int32_t _health_max = 12;

   float _stamina{1.0f};
   int32_t _stamina_drains{0};
};

/// \brief serializes persistent life and health values to json.
/// \param j json object receiving "lives", "health", and "health_max".
/// \param d health source data.
void to_json(nlohmann::json& j, const Health& d);

/// \brief deserializes persistent life and health values from json.
/// \param j json object containing serialized health fields.
/// \param d health target populated from json.
void from_json(const nlohmann::json& j, Health& d);
