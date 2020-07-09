#pragma once

#include <QImage>
#include "quad.h"
#include <functional>

struct PackTexture
{
   PackTexture() = default;

   bool load(const QString& filename);
   void pack();
   void dump();

   int size_ = 512;
   int textureSize_ = 0;
   QImage image_;
   QString filename_;
   std::vector<Quad> quads_;
   std::function<void(int)> updateProgress_;
};

