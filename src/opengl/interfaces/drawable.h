#pragma once

class Drawable
{
public:
   Drawable() = default;
   virtual ~Drawable() = default;

   virtual void render() const = 0;
};
