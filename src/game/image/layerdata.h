#ifndef LAYERDATA_H
#define LAYERDATA_H

#include "framework/image/layer.h"

struct LayerData
{
   LayerData(const std::shared_ptr<Layer>& layer);

   std::shared_ptr<Layer> _layer;
   sf::Vector2f _pos;
   float _alpha{1.0f};
};

#endif  // LAYERDATA_H
