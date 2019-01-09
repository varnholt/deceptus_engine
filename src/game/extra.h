#ifndef EXTRA_H
#define EXTRA_H


class Extra
{
public:

   enum class ExtraType
   {
      Invalid,
      Health,
      PowerJump,
      AirJump,
      Crouch
   };

   Extra() = default;

   ExtraType mExtraType = ExtraType::Invalid;
};

#endif // EXTRA_H
