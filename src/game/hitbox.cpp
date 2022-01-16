#include "hitbox.h"


Hitbox::Hitbox(const sf::FloatRect& rect)
 : _rect_px(rect)
{
}


sf::FloatRect Hitbox::getRectTranslated() const
{
   sf::FloatRect rect = _rect_px;
   rect.left += _offset_px.x;
   rect.top += _offset_px.y;
   return rect;
}
