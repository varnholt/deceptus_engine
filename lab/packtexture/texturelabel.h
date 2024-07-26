#pragma once

#include <QLabel>
#include <map>
#include "quad.h"

class TextureLabel : public QLabel
{
public:
   TextureLabel(QWidget* p = nullptr);

   QPixmap _pixmap;
   std::map<uint32_t, Quad>* _quads = nullptr;
   float _scale_x = 1.0f;
   float _scale_y = 1.0f;

protected:
   void paintEvent(QPaintEvent*);
};
