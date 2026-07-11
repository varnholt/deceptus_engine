#include "layerdata.h"

LayerData::LayerData(const std::shared_ptr<Layer>& layer)
#ifdef __EMSCRIPTEN__
    : _layer(layer), _pos(layer->_sprite->position), _alpha(layer->_sprite->color.a / 255.0f)
#else
    : _layer(layer), _pos(layer->_sprite->getPosition()), _alpha(layer->_sprite->getColor().a / 255.0f)
#endif
{
}
