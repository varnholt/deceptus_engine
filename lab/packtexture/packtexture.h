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

   int mSize = 512;
   int mTextureSize = 0;
   QImage mImage;
   QString mFilename;
   std::vector<Quad> mQuads;
   std::function<void(int)> mUpdateProgress;
};

