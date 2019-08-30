#pragma once

#include <memory>

// sfml
#include "SFML/Graphics.hpp"
#include "SFML/System.hpp"

#include "extra.h"
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
      KeyYellow = 65,
      KeyBlue   = 66,
      KeyGreen  = 67,
   };

   int mVertexOffset = 0;
   bool mActive = true;
   sf::Vector2u mSpriteOffset;
   sf::Vector2f mPosition;
   ExtraSpriteIndex mType;

   ExtraItem(GameNode* parent = nullptr);
};

