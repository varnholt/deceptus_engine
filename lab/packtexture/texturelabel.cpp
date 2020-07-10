#include "texturelabel.h"

#include <QPainter>

TextureLabel::TextureLabel(QWidget* p)
 : QLabel(p)
{
}


void TextureLabel::paintEvent(QPaintEvent * /*e*/)
{
   QPainter p(this);

   if (!mPixmap.isNull())
   {
      p.drawPixmap(0, 0, mPixmap);
   }

   if (mQuads != nullptr)
   {
      p.setBrush(QColor(255,0,0,60));
      std::for_each(mQuads->begin(), mQuads->end(), [&](Quad& q)
      {
         p.drawRect(
           q.mX * mScaleX,
           q.mY * mScaleY,
           q.mW * mScaleX,
           q.mH * mScaleY
         );
      });
   }
}
