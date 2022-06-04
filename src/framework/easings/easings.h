#pragma once

#include <cmath>
#include <functional>
#include <limits>
#include <math.h>
#include <type_traits>
#include <utility>


namespace Easings
{

enum class Type {
   None,

   EaseInSine,
   EaseInCubic,
   EaseInQuint,
   EaseInCirc,
   EaseInElastic,

   EaseOutSine,
   EaseOutCubic,
   EaseOutQuint,
   EaseOutCirc,
   EaseOutElastic,

   EaseInOutSine,
   EaseInOutCubic,
   EaseInOutQuint,
   EaseInOutCirc,
   EaseInOutElastic,
};

// linear
template<class T>
constexpr T linear(T x)
{
    return x;
}

// ease in
template<class T>
constexpr T easeInSine(T x)
{
   return static_cast<T>(1 - std::cos((x * M_PI) * 0.5));
}

template<class T>
constexpr T easeInCubic(T x)
{
   return x * x * x;
}

template<class T>
constexpr T easeInQuint(T x)
{
   return x * x * x * x * x;
}

template<class T>
constexpr T easeInCirc(T x)
{
   return static_cast<T>(1 - sqrt(1 - (x * x)));
}

template<class T>
constexpr T easeInElastic(T x)
{
   constexpr auto c4 = (2 * M_PI) / 3;
   return static_cast<T>((-std::pow(2, 10 * x - 10) * std::sin((x * 10 - 10.75) * c4)));
}


// ease out
template<class T>
constexpr T easeOutSine(T x)
{
   return static_cast<T>(std::sin((x * M_PI) * 0.5));
}

template<class T>
constexpr T easeOutCubic(T x)
{
   const auto c1 = 1 - x;
   const auto c2 = c1 * c1 * c1;
   return 1 - c2;
}

template<class T>
constexpr T easeOutQuint(T x)
{
   const auto c1 = 1 - x;
   const auto c2 = c1 * c1 * c1 * c1 * c1;
   return 1 - c2;
}

template<class T>
constexpr T easeOutCirc(T x)
{
   const auto c1 = x - 1;
   const auto c2 = c1 * c1;
   return static_cast<T>(sqrt(1 - c2));
}

template<class T>
constexpr T easeOutElastic(T x)
{
   constexpr auto c4 = (2 * M_PI) / 3;
   return static_cast<T>(std::pow(2, -10 * x) * std::sin((x * 10 - 0.75) * c4) + 1);
}


// ease in/out
template<class T>
constexpr T easeInOutSine(T x)
{
   return static_cast<T>(-(std::cos(M_PI * x) - 1) * 0.5);
}

template<class T>
constexpr T easeInOutCubic(T x)
{
   if (x < 0.5)
   {
      return 4 * x * x * x;
   }

   const auto c1 = -2 * x + 2;
   return static_cast<T>(1 - c1 * c1 * c1 * 0.5);
}

template<class T>
constexpr T easeInOutQuint(T x)
{
   if (x < 0.5)
   {
      return 16 * x * x * x * x * x;
   }

   const auto c1 = -2 * x + 2;
   return (1 - c1 * c1 * c1 * c1 * c1 * T{0.5});
}

template<class T>
constexpr T easeInOutCirc(T x)
{
   if (x < 0.5)
   {
      const auto c1 = 2 * x;
      return static_cast<T>((1 - sqrt(1 - c1 * c1)) * 0.5);
   }

   const auto c1 = -2 * x + 2;
   return static_cast<T>((sqrt(1 - c1 * c1) + 1) * 0.5);
}

template<class T>
constexpr T easeInOutElastic(T x)
{
   constexpr auto c5 = (2 * M_PI) / 4.5;

   if (x < 0.5)
   {
      return static_cast<T>(-(std::pow(2,  20 * x - 10) * std::sin((20 * x - 11.125) * c5)) * 0.5);
   }

   return static_cast<T>((std::pow(2, -20 * x + 10) * std::sin((20 * x - 11.125) * c5)) * 0.5 + 1);
}


// ease in
template<class T>
constexpr T easeInQuad(T x)
{
   return x * x;
}

template<class T>
constexpr T easeInQuart(T x)
{
   return x * x * x * x;
}

template<class T>
constexpr T easeInExpo(T x)
{
   return static_cast<T>(std::pow(2, 10 * x - 10));
}

template<class T>
constexpr T easeInBack(T x)
{
   constexpr auto c1 = 1.70158;
   constexpr auto c3 = c1 + 1;
   return static_cast<T>(c3 * x * x * x - c1 * x * x);
}

template<class T>
constexpr T easeInBounce(T x)
{
   return 1 - easeOutBounce(1 - x);
}


// ease out

template<class T>
constexpr T easeOutBack(T x)
{
   constexpr auto c1 = 1.70158;
   constexpr auto c3 = c1 + 1;
   const auto c2 = x - 1;

   return static_cast<T>(1 + c3 * c2 * c2 * c2 + c1 * c2 * c2);
}

template<class T>
constexpr T easeOutQuad(T x)
{
   return 1 - (1 - x) * (1 - x);
}

template<class T>
constexpr T easeOutQuart(T x)
{
   const auto c1 = 1 - x;
   return 1 - c1 * c1 * c1 * c1;
}

template<class T>
constexpr T easeOutExpo(T x)
{
   return static_cast<T>(1 - std::pow(2, -10 * x));
}

template<class T>
constexpr T easeOutBounce(T x)
{
   constexpr T n1 = 7.5625;
   constexpr T d1 = 2.75;

   if (x < 1.0 / d1)
   {
      return n1 * x * x;
   }
   else if (x < 2.0 / d1)
   {
      return n1 * (x -= T{1.5} / d1) * x + T{0.75};
   }
   else if (x < 2.5 / d1)
   {
      return n1 * (x -= T{2.25} / d1) * x + T{0.9375};
   }
   else
   {
      return n1 * (x -= T{2.625} / d1) * x + T{0.984375};
   }
}


// ease in/out

template<class T>
constexpr T easeInOutQuad(T x)
{
   if (x < 0.5)
   {
      return 2 * x * x;
   }

   const auto c1 = -2 * x + 2;
   return 1 - c1 * c1 * T{0.5};
}

template<class T>
constexpr T easeInOutQuart(T x)
{
   if (x < 0.5)
   {
      return 8 * x * x * x * x;
   }

   const auto c1 = -2 * x + 2;
   return 1 - c1 * c1 * c1 * c1 * T{0.5};
}

template<class T>
constexpr T easeInOutExpo(T x)
{
   if (x < 0.5)
   {
      return static_cast<T>(std::pow(2, 20 * x - 10) * 0.5);
   }

   return static_cast<T>((2 - std::pow(2, -20 * x + 10)) * 0.5);
}

template<class T>
constexpr T easeInOutBack(T x)
{
   constexpr auto c1 = 1.70158;
   constexpr auto c2 = c1 * 1.525;

   if (x < 0.5)
   {
      const auto c3 = 2 * x;
      return static_cast<T>((c3 * c3 * ((c2 + 1) * 2 * x - c2)) * 0.5);
   }

   const auto c3 = 2 * x - 2;
   return static_cast<T>((c3 * c3 * ((c2 + 1) * (x * 2 - 2) + c2) + 2) * 0.5);
}

template<class T>
constexpr T easeInOutBounce(T x)
{
   if (x < 0.5)
   {
      return static_cast<T>((1 - easeOutBounce(1 - 2 * x)) * 0.5);
   }

   return static_cast<T>((1 + easeOutBounce(2 * x - 1)) * 0.5);
}

template<class T>
std::function<T(T)> getFunction(Type type)
{
   switch (type)
   {
      case Type::EaseInSine:
         return [](T x){return Easings::easeInSine(x);};
      case Type::EaseInCubic:
         return [](T x){return Easings::easeInCubic(x);};
      case Type::EaseInQuint:
         return [](T x){return Easings::easeInQuint(x);};
      case Type::EaseInCirc:
         return [](T x){return Easings::easeInCirc(x);};
      case Type::EaseInElastic:
         return [](T x){return Easings::easeInElastic(x);};
      case Type::EaseOutSine:
         return [](T x){return Easings::easeOutSine(x);};
      case Type::EaseOutCubic:
         return [](T x){return Easings::easeOutCubic(x);};
      case Type::EaseOutQuint:
         return [](T x){return Easings::easeOutQuint(x);};
      case Type::EaseOutCirc:
         return [](T x){return Easings::easeOutCirc(x);};
      case Type::EaseOutElastic:
         return [](T x){return Easings::easeOutElastic(x);};
      case Type::EaseInOutSine:
         return [](T x){return Easings::easeInOutSine(x);};
      case Type::EaseInOutCubic:
         return [](T x){return Easings::easeInOutCubic(x);};
      case Type::EaseInOutQuint:
         return [](T x){return Easings::easeInOutQuint(x);};
      case Type::EaseInOutCirc:
         return [](T x){return Easings::easeInOutCirc(x);};
      case Type::EaseInOutElastic:
         return [](T x){return Easings::easeInOutElastic(x);};
      case Type::None:
         return [](T x){return Easings::linear(x);};
   }

   return [](T x){return Easings::linear(x);};
}

}

