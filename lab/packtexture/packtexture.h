#pragma once

#include <QImage>
#include <functional>
#include <map>
#include <vector>
#include "quad.h"

struct PackTexture
{
   PackTexture() = default;

   bool load(const QString& filename);
   void pack();
   void dump();

   int _size = 512;
   int _texture_size = 0;
   QImage _image;
   QString _filename;
   std::map<uint32_t, Quad> _quads;
   std::function<void(int)> _update_progress;
   std::function<void(const QString&)> _log;
};
