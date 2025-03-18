#include "hitbox.h"

Hitbox::Hitbox(const sf::FloatRect& rect, const sf::Vector2f& offset) : _rect_px(rect), _offset_px(offset)
{
}

sf::FloatRect Hitbox::getRectTranslated() const
{
   sf::FloatRect rect = _rect_px;
   rect.position.x += _offset_px.x;
   rect.position.y += _offset_px.y;
   return rect;
}
