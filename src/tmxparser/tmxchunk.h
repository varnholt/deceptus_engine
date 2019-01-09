#pragma once

#include "tmxelement.h"

// Qt
#include "SFML/Graphics.hpp"

struct TmxChunk : TmxElement
{

  TmxChunk() = default;
  ~TmxChunk();

  void deserialize(tinyxml2::XMLElement* element);
  std::vector<sf::Vector2f> mPolyLine;
  int32_t mX = 0;
  int32_t mY = 0;
  int32_t mWidth = 0;
  int32_t mHeight = 0;
  int32_t* mData = nullptr;
};

