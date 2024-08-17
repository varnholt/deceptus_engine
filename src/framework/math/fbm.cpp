#include "fbm.h"
#include <cmath>
#include <iostream>

// code is based on
// - https://thebookofshaders.com/13/
// - https://www.shadertoy.com/view/4dS3Wd

constexpr auto OCTAVES = 6;
constexpr auto WIDTH = 512;
constexpr auto HEIGHT = 512;

float fbm::dot(vec2 v1, vec2 v2)
{
   return v1.x * v2.x + v1.y * v2.y;
}

float fbm::fract(float x)
{
   return x - floor(x);
}

float fbm::mix(float a, float b, float x)
{
   return a * (1.0f - x) + b * x;
}

float fbm::hash(float n)
{
   return fract(sin(n) * 1e4f);
}

float fbm::hash(const vec2& p)
{
   return fract(1e4f * sin(17.0f * p.x + p.y * 0.1f) * (0.1f + abs(sin(p.y * 13.0f + p.x))));
}

float fbm::noise2(const vec2& st)
{
   vec2 i{floor(st.x), floor(st.y)};
   vec2 f{fract(st.x), fract(st.y)};

   // four corners in 2D of a tile
   auto a = hash(i);
   auto b = hash(i + vec2{1.0f, 0.0f});
   auto c = hash(i + vec2{0.0f, 1.0f});
   auto d = hash(i + vec2{1.0f, 1.0f});

   vec2 u = f * f * (vec2{3.0f, 3.0f} - (f * 2.0f));

   return mix(a, b, u.x) + (c - a) * u.y * (1.0f - u.x) + (d - b) * u.x * u.y;
}

float fbm::random1(const vec2& st)
{
   return fract(sin(dot(st, vec2{12.9898f, 78.233f})) * 43758.5453123f);
}

float fbm::noise1(const vec2& st)
{
   vec2 i{floor(st.x), floor(st.y)};
   vec2 f{fract(st.x), fract(st.y)};

   // four corners in 2D of a tile
   auto a = random1(i);
   auto b = random1(i + vec2{1.0f, 0.0f});
   auto c = random1(i + vec2{0.0f, 1.0f});
   auto d = random1(i + vec2{1.0f, 1.0f});

   vec2 u = f * f * (vec2{3.0f, 3.0f} - (f * 2.0f));

   return mix(a, b, u.x) + (c - a) * u.y * (1.0f - u.x) + (d - b) * u.x * u.y;
}

float fbm::fbm(vec2 st)
{
   auto value = 0.0f;
   auto amplitude = 0.5f;

   for (auto i = 0; i < OCTAVES; i++)
   {
      value += amplitude * noise2(st);
      st *= 2.0f;
      amplitude *= 0.5f;
   }
   return value;
}

void fbm::test()
{
   for (auto x = 0; x < 512; x++)
   {
      vec2 pos{static_cast<float>(x), 0.0f};
      vec2 resolution{WIDTH, HEIGHT};

      vec2 st = pos / resolution;
      st.x *= resolution.x / resolution.y;

      auto test_value = fbm(st * 3.0f);

      std::cout << test_value << std::endl;
   }
}
