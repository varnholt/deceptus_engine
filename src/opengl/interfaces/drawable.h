#pragma once

/// \brief base interface for objects that can issue draw calls.
class Drawable
{
public:
   Drawable() = default;
   virtual ~Drawable() = default;

   /// \brief renders the object with its internally prepared OpenGL state.
   virtual void render() const = 0;
};
