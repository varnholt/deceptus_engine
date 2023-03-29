#ifndef FAKESFML_H
#define FAKESFML_H

namespace sf
{
struct Vector2f
{
   Vector2f() = default;
   Vector2f(float x, float y);
   float x{0.0f};
   float y{0.0f};

   Vector2f operator+=(const Vector2f&);
   Vector2f operator+(const Vector2f&);
   Vector2f operator*(float val);
};
}  // namespace sf

#endif  // FAKESFML_H
