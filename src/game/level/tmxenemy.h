#pragma once

#include "box2d/box2d.h"
#include <SFML/Graphics.hpp>

#include <array>
#include <optional>
#include <string>

#include "constants.h"
#include "scriptproperty.h"

struct TmxObject;

/// \brief parsed TMX object data used to spawn and configure a scripted enemy.
struct TmxEnemy
{
   TmxEnemy() = default;

   /// \brief parses a TMX object into enemy fields, properties, bounds, and optional path points.
   /// \param object TMX object that defines the enemy.
   void parse(const std::shared_ptr<TmxObject>& object);

   /// \brief assigns a physics path by matching this enemy rect against path chains.
   /// \param paths candidate world paths in box2d coordinates.
   void addPaths(const std::vector<std::vector<b2Vec2>>& paths);

   /// \brief looks up a parsed property by key.
   /// \param key property name.
   /// \return matching property, or std::nullopt when not found.
   std::optional<ScriptProperty> findProperty(const std::string& key);

   sf::Vector2i _pixel_position;
   std::string _id;
   std::string _name;
   sf::IntRect _pixel_rect;
   std::array<sf::Vector2i, 4> _vertices;
   std::vector<b2Vec2> _path;
   std::vector<int32_t> _pixel_path;
   bool _has_path = false;
   bool _inverse_path = false;
   std::vector<ScriptProperty> _properties;
   Winding _winding = Winding::Clockwise;
};
