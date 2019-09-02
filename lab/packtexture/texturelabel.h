#ifndef TEXTURELABEL_H
#define TEXTURELABEL_H

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

  TextureLabel(QWidget* p = 0);

protected:

  void paintEvent(QPaintEvent *);


};

#endif // TEXTURELABEL_H
