#pragma once

#include "tmxelement.h"

struct TmxImage;
struct TmxProperties;

struct TmxImageLayer : public TmxElement
{
  TmxImageLayer();

  float mOffsetX = 0.0f;
  float mOffsetY = 0.0f;
  float mOpacity = 1.0f;
  TmxImage* mImage = nullptr;
  TmxProperties* mProperties = nullptr;
  int32_t mZ = 0;

  void deserialize(tinyxml2::XMLElement *e);
};

