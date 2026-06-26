#include "layerdata.h"

LayerData::LayerData(const std::shared_ptr<Layer>& layer)
    : _layer(layer), _pos(layer->_sprite->position), _alpha(layer->_sprite->color.a / 255.0f)
{
}
