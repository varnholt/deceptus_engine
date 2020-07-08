#include "texturelabel.h"

#include <QPainter>

TextureLabel::TextureLabel(QWidget* p)
 : QLabel(p)
{
}


void TextureLabel::paintEvent(QPaintEvent * /*e*/)
{
   QPainter p(this);

   if (!pixmap_.isNull())
   {
      p.drawPixmap(0, 0, pixmap_);
   }

   if (quads_ != nullptr)
   {
      p.setBrush(QColor(255,0,0,60));
      std::for_each(quads_->begin(), quads_->end(), [&](Quad& q)
      {
         p.drawRect(
           q.x_ * scaleX_,
           q.y_ * scaleY_,
           q.w_ * scaleX_,
           q.h_ * scaleY_
         );
      });
   }
}
