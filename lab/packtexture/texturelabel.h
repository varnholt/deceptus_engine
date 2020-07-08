#pragma once

#include <QLabel>
#include "quad.h"

class TextureLabel : public QLabel
{

public:
   QPixmap pixmap_;
   std::vector<Quad>* quads_ = nullptr;
   float scaleX_ = 1.0f;
   float scaleY_ = 1.0f;


public:
   TextureLabel(QWidget* p = nullptr);

protected:

   void paintEvent(QPaintEvent *);

};

