#pragma once

#include <memory>

// sfml
#include "SFML/Graphics.hpp"
#include "SFML/System.hpp"

#include "gamenode.h"
#include "tmxparser/tmxtile.h"


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
   };

   bool mActive = true;
   sf::Vector2u mSpriteOffset;
   sf::Vector2f mPosition;
   ExtraSpriteIndex mType;

   ExtraItem(GameNode* parent = nullptr);
};

