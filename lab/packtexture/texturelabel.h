#pragma once

#include <QLabel>
#include "quad.h"

class TextureLabel : public QLabel
{

public:
   QPixmap mPixmap;
   std::vector<Quad>* mQuads = nullptr;
   float mScaleX = 1.0f;
   float mScaleY = 1.0f;


public:
   TextureLabel(QWidget* p = nullptr);

protected:

   void paintEvent(QPaintEvent *);

};

