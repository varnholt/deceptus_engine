#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "json/json.hpp"

class Weapon;
struct b2Body;

/// \brief stores the player's owned weapons and currently selected weapon.
struct WeaponSystem
{
   /// \brief creates an empty weapon inventory.
   WeaponSystem() = default;

   std::shared_ptr<Weapon> _selected;
   std::vector<std::shared_ptr<Weapon>> _weapons;
};

/// \brief serializes weapon names and selected weapon name to json.
/// \param j json object receiving the serialized weapon system state.
/// \param d weapon system source data.
void to_json(nlohmann::json& j, const WeaponSystem& d);
/// \brief deserializes weapon names, recreates weapon instances, and restores selection.
/// \param j json object with "weapons" and optional "selected" fields.
/// \param d weapon system target populated from json.
void from_json(const nlohmann::json& j, WeaponSystem& d);
