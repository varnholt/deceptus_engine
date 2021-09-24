#pragma once

#include <memory>

// sfml
#include "SFML/Graphics.hpp"
#include "SFML/System.hpp"

#include "gamenode.h"
#include "framework/tmxparser/tmxtile.h"


struct ExtraItem : public GameNode
{
   enum class ExtraSpriteIndex
   {
      Coin      =  0,
      Cherry    = 16,
      Banana    = 32,
      Apple     = 48,
      KeyRed    = 64,
      KeyOrange = 65,
      KeyBlue   = 66,
      KeyGreen  = 67,
      KeyYellow = 68,
      Dash      = 80,
      Invalid   = 0xffff
   };

   ExtraItem(GameNode* parent = nullptr);

   bool _active = true;
   sf::Vector2u _sprite_offset;
   sf::Vector2f _position;
   ExtraSpriteIndex _type = ExtraSpriteIndex::Invalid;
};

