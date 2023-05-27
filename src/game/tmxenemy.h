#pragma once

#include <SFML/Graphics.hpp>
#include <Box2D/Box2D.h>

#include <array>
#include <optional>
#include <string>

#include "constants.h"
#include "scriptproperty.h"

struct TmxObject;

struct TmxEnemy
{
   TmxEnemy() = default;

   void parse(const std::shared_ptr<TmxObject>& object);
   void addPaths(const std::vector<std::vector<b2Vec2>>& paths);

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

