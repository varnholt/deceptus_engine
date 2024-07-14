#include "layerdata.h"

LayerData::LayerData(const std::shared_ptr<Layer>& layer)
    : _layer(layer), _pos(layer->_sprite->getPosition()), _alpha(layer->_sprite->getColor().a / 255.0f)
{
}
