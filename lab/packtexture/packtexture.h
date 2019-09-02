#ifndef PACKTEXTURE_H
#define PACKTEXTURE_H

#include <QImage>
#include "quad.h"
#include <functional>

class PackTexture
{

public:

  int size_ = 512;
  int textureSize_ = 0;
  QImage image_;
  QString filename_;
  std::vector<Quad> quads_;
  std::function<void(int)> updateProgress_;

public:

  PackTexture() = default;

  void load(const QString& filename);
  void pack();
  void dump();
};

#endif // PACKTEXTURE_H
