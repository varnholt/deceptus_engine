#include "texturelabel.h"

#include <QPainter>

TextureLabel::TextureLabel(QWidget* p) : QLabel(p)
{
}

void TextureLabel::paintEvent(QPaintEvent* /*e*/)
{
   QPainter p(this);

   if (!_pixmap.isNull())
   {
      p.drawPixmap(0, 0, _pixmap);
   }

   if (_quads != nullptr)
   {
      for (auto& [k, q] : *_quads)
      {
         auto unique_quad = true;
         for (auto& rect : q._rects)
         {
            p.setBrush(QColor(unique_quad ? 255 : 0, 0, unique_quad ? 0 : 255, 60));
            unique_quad = false;

            p.drawRect(
               rect._x / _scale_x,  //
               rect._y / _scale_y,
               rect._w / _scale_x,
               rect._h / _scale_y
            );
         }
      };
   }
}
