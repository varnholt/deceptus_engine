#pragma once

#include "tmxelement.h"

#include "SFML/Graphics.hpp"

struct TmxChunk : TmxElement
{

  TmxChunk() = default;
  ~TmxChunk() override;

  void deserialize(tinyxml2::XMLElement* element, const std::shared_ptr<TmxParseData>&) override;
  std::vector<sf::Vector2f> _polyline;
  int32_t _x_px = 0;
  int32_t _y_px = 0;
  int32_t _width_px = 0;
  int32_t _height_px = 0;
  int32_t* _data = nullptr;
};

