#ifndef LAYERDATA_H
#define LAYERDATA_H

#include "framework/image/layer.h"

/// \brief captures a layer together with its initial position and alpha value.
struct LayerData
{
   /// \brief snapshots the current sprite position and opacity of a layer.
   /// \param layer layer whose sprite state is cached for menu animations.
   LayerData(const std::shared_ptr<Layer>& layer);

   std::shared_ptr<Layer> _layer;
   sf::Vector2f _pos;
   float _alpha{1.0f};
};

#endif  // LAYERDATA_H
